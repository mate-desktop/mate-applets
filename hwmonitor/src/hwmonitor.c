#include <config.h>

#include <atk/atk.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

#include "hwmonitor-sensors.h"

#define HW_SCHEMA "org.mate.panel.applet.hwmonitor"

typedef struct
{
    MatePanelApplet *applet;
    GSettings *settings;
    GtkLabel *label;
    guint timeout_id;
    guint64 previous_total;
    guint64 previous_idle;
    HwMonitorSnapshot snapshot;
} HardwareMonitorApplet;

static void hardware_monitor_update (HardwareMonitorApplet *monitor);
static void hardware_monitor_preferences (GtkAction *action, HardwareMonitorApplet *monitor);
static void hardware_monitor_about (GtkAction *action, HardwareMonitorApplet *monitor);

static const GtkActionEntry applet_menu_actions[] = {
    { "Preferences", "document-properties", N_("_Preferences"), NULL, NULL,
      G_CALLBACK (hardware_monitor_preferences) },
    { "About", "help-about", N_("_About"), NULL, NULL,
      G_CALLBACK (hardware_monitor_about) }
};

static const gchar *menu_ui =
    "<menuitem name='Preferences Item' action='Preferences' />"
    "<menuitem name='About Item' action='About' />";

static gchar *format_temperature (gdouble celsius, const gchar *unit)
{
    if (g_strcmp0 (unit, "fahrenheit") == 0)
        return g_strdup_printf ("%.0f°F", celsius * 9.0 / 5.0 + 32.0);
    return g_strdup_printf ("%.0f°C", celsius);
}

static void append_metric (GString *text, const gchar *metric)
{
    if (text->len > 0)
        g_string_append (text, " | ");
    g_string_append (text, metric);
}

static void append_gpu (GString *text, const HwMonitorGpu *gpu, guint index,
                        GSettings *settings, const gchar *unit)
{
    gchar *temperature;
    gchar *metric;
    gboolean want_temperature = index == 0 ? g_settings_get_boolean (settings, "show-gpu0-temperature") : g_settings_get_boolean (settings, "show-gpu1-temperature");
    gboolean want_usage = index == 0 ? g_settings_get_boolean (settings, "show-gpu0-usage") : g_settings_get_boolean (settings, "show-gpu1-usage");
    gboolean want_vram = index == 0 ? g_settings_get_boolean (settings, "show-gpu0-vram") : g_settings_get_boolean (settings, "show-gpu1-vram");

    if (!gpu->present)
        return;
    if (want_temperature) {
        temperature = gpu->have_temperature ? format_temperature (gpu->temperature_c, unit) : g_strdup (_("--"));
        metric = g_strdup_printf ("GPU%u %s", index, temperature);
        append_metric (text, metric);
        g_free (metric); g_free (temperature);
    }
    if (want_usage) {
        metric = gpu->have_usage ? g_strdup_printf ("GPU%u %.0f%%", index, gpu->usage_percent) : g_strdup_printf ("GPU%u --", index);
        append_metric (text, metric);
        g_free (metric);
    }
    if (want_vram) {
        metric = gpu->have_vram ? g_strdup_printf ("GPU%u VRAM %.1fG", index, gpu->vram_free_mib / 1024.0) : g_strdup_printf ("GPU%u VRAM --", index);
        append_metric (text, metric);
        g_free (metric);
    }
}

static void hardware_monitor_render (HardwareMonitorApplet *monitor)
{
    GString *text = g_string_new (NULL);
    const gchar *unit = g_settings_get_string (monitor->settings, "temperature-unit");
    gchar *temperature;
    gchar *metric;

    if (g_settings_get_boolean (monitor->settings, "show-cpu-temperature")) {
        temperature = monitor->snapshot.have_cpu_temperature ? format_temperature (monitor->snapshot.cpu_temperature_c, unit) : g_strdup (_("--"));
        metric = g_strdup_printf ("CPU %s", temperature);
        append_metric (text, metric);
        g_free (metric); g_free (temperature);
    }
    if (g_settings_get_boolean (monitor->settings, "show-cpu-usage")) {
        metric = monitor->snapshot.have_cpu_usage ? g_strdup_printf ("CPU %.0f%%", monitor->snapshot.cpu_usage_percent) : g_strdup_printf ("CPU --");
        append_metric (text, metric);
        g_free (metric);
    }
    if (g_settings_get_boolean (monitor->settings, "show-memory-available")) {
        metric = monitor->snapshot.have_memory ? g_strdup_printf ("RAM libre %.1fG", monitor->snapshot.memory_available_mib / 1024.0) : g_strdup_printf ("RAM libre --");
        append_metric (text, metric);
        g_free (metric);
    }
    append_gpu (text, &monitor->snapshot.gpu[0], 0, monitor->settings, unit);
    append_gpu (text, &monitor->snapshot.gpu[1], 1, monitor->settings, unit);
    g_free ((gchar *) unit);

    gtk_label_set_text (monitor->label, text->len > 0 ? text->str : _("No metrics selected"));
    gtk_widget_set_tooltip_text (GTK_WIDGET (monitor->label), text->str);
    g_string_free (text, TRUE);
}

static gboolean hardware_monitor_timeout (HardwareMonitorApplet *monitor)
{
    hardware_monitor_update (monitor);
    return G_SOURCE_CONTINUE;
}

static void hardware_monitor_update (HardwareMonitorApplet *monitor)
{
    hwmonitor_collect (&monitor->snapshot, &monitor->previous_total, &monitor->previous_idle);
    hardware_monitor_render (monitor);
}

static void hardware_monitor_restart_timer (HardwareMonitorApplet *monitor)
{
    guint interval;
    if (monitor->timeout_id != 0)
        g_source_remove (monitor->timeout_id);
    interval = g_settings_get_uint (monitor->settings, "update-interval");
    monitor->timeout_id = g_timeout_add (MAX (250u, interval), (GSourceFunc) hardware_monitor_timeout, monitor);
}

static void hardware_monitor_settings_changed (GSettings *settings, gchar *key, HardwareMonitorApplet *monitor)
{
    if (g_strcmp0 (key, "update-interval") == 0)
        hardware_monitor_restart_timer (monitor);
    hardware_monitor_render (monitor);
}

static GtkWidget *add_check (GtkGrid *grid, const gchar *label, const gchar *key, guint row, GSettings *settings)
{
    GtkWidget *check = gtk_check_button_new_with_mnemonic (label);
    gtk_grid_attach (grid, check, row % 2, row / 2, 1, 1);
    g_settings_bind (settings, key, check, "active", G_SETTINGS_BIND_DEFAULT);
    return check;
}

static void hardware_monitor_preferences (GtkAction *action, HardwareMonitorApplet *monitor)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Hardware Monitor Preferences"), NULL,
                                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                                       _("_Close"), GTK_RESPONSE_CLOSE, NULL);
    GtkWidget *content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkWidget *grid = gtk_grid_new ();
    GtkWidget *interval = gtk_spin_button_new_with_range (250, 10000, 250);
    GtkWidget *unit = gtk_combo_box_text_new ();

    gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
    gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
    gtk_grid_set_column_spacing (GTK_GRID (grid), 18);
    gtk_box_pack_start (GTK_BOX (content), grid, TRUE, TRUE, 0);
    gtk_grid_attach (GTK_GRID (grid), gtk_label_new (_("Update interval (ms):")), 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), interval, 1, 0, 1, 1);
    g_settings_bind (monitor->settings, "update-interval", interval, "value", G_SETTINGS_BIND_DEFAULT);
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (unit), "celsius", _("Celsius"));
    gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (unit), "fahrenheit", _("Fahrenheit"));
    gtk_grid_attach (GTK_GRID (grid), gtk_label_new (_("Temperature unit:")), 0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grid), unit, 1, 1, 1, 1);
    g_settings_bind (monitor->settings, "temperature-unit", unit, "active-id", G_SETTINGS_BIND_DEFAULT);
    add_check (GTK_GRID (grid), _("CPU temperature"), "show-cpu-temperature", 4, monitor->settings);
    add_check (GTK_GRID (grid), _("CPU usage"), "show-cpu-usage", 5, monitor->settings);
    add_check (GTK_GRID (grid), _("Available RAM"), "show-memory-available", 6, monitor->settings);
    add_check (GTK_GRID (grid), _("GPU 0 temperature"), "show-gpu0-temperature", 7, monitor->settings);
    add_check (GTK_GRID (grid), _("GPU 0 usage"), "show-gpu0-usage", 8, monitor->settings);
    add_check (GTK_GRID (grid), _("GPU 0 free VRAM"), "show-gpu0-vram", 9, monitor->settings);
    add_check (GTK_GRID (grid), _("GPU 1 temperature"), "show-gpu1-temperature", 10, monitor->settings);
    add_check (GTK_GRID (grid), _("GPU 1 usage"), "show-gpu1-usage", 11, monitor->settings);
    add_check (GTK_GRID (grid), _("GPU 1 free VRAM"), "show-gpu1-vram", 12, monitor->settings);
    g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
    gtk_widget_show_all (dialog);
}

static void hardware_monitor_about (GtkAction *action, HardwareMonitorApplet *monitor)
{
    const gchar *authors[] = { "MATE developers", NULL };
    gtk_show_about_dialog (NULL, "program-name", _("Hardware Monitor"), "version", VERSION,
                           "comments", _("Shows CPU, memory and GPU telemetry"),
                           "copyright", "Copyright © 2026 MATE developers",
                           "authors", authors, "logo-icon-name", "utilities-system-monitor",
                           "translator-credits", _("translator-credits"), NULL);
}

static void hardware_monitor_destroy (MatePanelApplet *applet, HardwareMonitorApplet *monitor)
{
    if (monitor->timeout_id != 0)
        g_source_remove (monitor->timeout_id);
    g_clear_object (&monitor->settings);
    g_free (monitor);
}

static gboolean hardware_monitor_fill (MatePanelApplet *applet)
{
    HardwareMonitorApplet *monitor = g_new0 (HardwareMonitorApplet, 1);
    GtkActionGroup *actions;
    AtkObject *accessible;

#ifndef ENABLE_IN_PROCESS
    g_set_application_name (_("Hardware Monitor"));
#endif
    gtk_window_set_default_icon_name ("utilities-system-monitor");
    monitor->applet = applet;
    monitor->settings = mate_panel_applet_settings_new (applet, HW_SCHEMA);
    monitor->label = GTK_LABEL (gtk_label_new (_("Reading sensors…")));
    gtk_label_set_xalign (monitor->label, 0.0);
    gtk_container_add (GTK_CONTAINER (applet), GTK_WIDGET (monitor->label));
    mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);
    gtk_widget_show_all (GTK_WIDGET (applet));
    g_signal_connect (applet, "destroy", G_CALLBACK (hardware_monitor_destroy), monitor);
    g_signal_connect (monitor->settings, "changed", G_CALLBACK (hardware_monitor_settings_changed), monitor);
    actions = gtk_action_group_new ("Hardware Monitor Actions");
    gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (actions, applet_menu_actions, G_N_ELEMENTS (applet_menu_actions), monitor);
    mate_panel_applet_setup_menu (applet, menu_ui, actions);
    accessible = gtk_widget_get_accessible (GTK_WIDGET (applet));
    if (GTK_IS_ACCESSIBLE (accessible)) {
        atk_object_set_name (accessible, _("Hardware Monitor applet"));
        atk_object_set_description (accessible, _("Shows CPU, memory and GPU telemetry"));
    }
    hardware_monitor_update (monitor);
    hardware_monitor_restart_timer (monitor);
    return TRUE;
}

static gboolean hardware_monitor_factory (MatePanelApplet *applet, const gchar *iid, gpointer data)
{
    return g_strcmp0 (iid, "HardwareMonitorApplet") == 0 && hardware_monitor_fill (applet);
}

PANEL_APPLET_FACTORY ("HardwareMonitorAppletFactory", PANEL_TYPE_APPLET,
                      "Hardware Monitor applet", hardware_monitor_factory, NULL)
