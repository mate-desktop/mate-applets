/* MATE cpuload/memload panel applet
 * (C) 2002 The Free Software Foundation
 *
 * Authors:
 *          Todd Kulesza
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gtk/gtk.h>

#include <gio/gio.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

#include "global.h"

#define NEVER_SENSITIVE       "never_sensitive"

/* set sensitive and setup NEVER_SENSITIVE appropriately */
static void
hard_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
    gtk_widget_set_sensitive (w, sensitivity);
    g_object_set_data (G_OBJECT (w), NEVER_SENSITIVE,
                       GINT_TO_POINTER ( ! sensitivity));
}

/* set sensitive, but always insensitive if NEVER_SENSITIVE is set */
static void
soft_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
    if (g_object_get_data (G_OBJECT (w), NEVER_SENSITIVE))
        gtk_widget_set_sensitive (w, FALSE);
    else
        gtk_widget_set_sensitive (w, sensitivity);
}

static void
properties_set_insensitive(MultiloadApplet *ma)
{
    guint total_graphs = 0;
    guint last_graph = 0;
    guint i;

    for (i = 0; i < graph_n; i++)
        if (ma->graphs[i]->visible)
        {
            last_graph = i;
            total_graphs++;
        }

    if (total_graphs < 2)
        soft_set_sensitive (ma->check_boxes[last_graph], FALSE);
}

static void
on_properties_dialog_response (GtkWidget       *widget,
                               gint             arg,
                               MultiloadApplet *ma)
{
    GError *error = NULL;

    switch (arg) {
        case GTK_RESPONSE_HELP:

            gtk_show_uri_on_window (NULL,
                                    "help:mate-multiload/multiload-prefs",
                                    gtk_get_current_event_time (),
                                    &error);

            if (error) { /* FIXME: the user needs to see this */
                g_warning ("help error: %s\n", error->message);
                g_error_free (error);
            }
            break;

        case GTK_RESPONSE_CLOSE:
        default:
            gtk_widget_destroy (widget);
            ma->prop_dialog = NULL;
    }
}

static void
on_speed_spin_button_value_changed (GtkSpinButton *spin_button,
                                    gpointer       user_data)
{
    MultiloadApplet *ma = user_data;
    gint value;
    guint i;

    value = gtk_spin_button_get_value_as_int (spin_button);
    g_settings_set_uint (ma->settings, REFRESH_RATE_KEY, (guint) value);
    for (i = 0; i < graph_n; i++) {
        load_graph_stop (ma->graphs[i]);
        ma->graphs[i]->speed = (guint) value;
        if (ma->graphs[i]->visible)
            load_graph_start (ma->graphs[i]);
    }
}

static void
on_graph_size_spin_button_value_changed (GtkSpinButton *spin_button,
                                         gpointer       user_data)
{
    MultiloadApplet *ma = user_data;
    gint value;
    guint i;

    value = gtk_spin_button_get_value_as_int (spin_button);
    g_settings_set_uint (ma->settings, GRAPH_SIZE_KEY, (guint) value);
    for (i = 0; i < graph_n; i++) {
        ma->graphs[i]->size = (guint) value;
        if (ma->graphs[i]->orient) {
            gtk_widget_set_size_request (ma->graphs[i]->main_widget,
                                         (gint) ma->graphs[i]->pixel_size,
                                         (gint) ma->graphs[i]->size);
        } else {
            gtk_widget_set_size_request (ma->graphs[i]->main_widget,
                                         (gint) ma->graphs[i]->size,
                                         (gint) ma->graphs[i]->pixel_size);
        }
    }
}

static void
on_net_threshold1_spin_button_value_changed (GtkSpinButton *spin_button,
                                             gpointer       user_data)
{
    MultiloadApplet *ma = user_data;
    gdouble temp;

    temp = gtk_spin_button_get_value (spin_button);
    ma->net_threshold1 = (guint64) temp;
    if (ma->net_threshold1 >= ma->net_threshold2) {
        ma->net_threshold1 = ma->net_threshold2 - 1;
        gtk_spin_button_set_value (spin_button, (gdouble) ma->net_threshold1);
    }
    g_settings_set_uint64 (ma->settings, KEY_NET_THRESHOLD1, ma->net_threshold1);
}

static void
on_net_threshold2_spin_button_value_changed (GtkSpinButton *spin_button,
                                             gpointer       user_data)
{
    MultiloadApplet *ma = user_data;
    gdouble temp;

    temp = gtk_spin_button_get_value (spin_button);
    ma->net_threshold2 = (guint64) temp;
    if (ma->net_threshold2 >= ma->net_threshold3) {
        ma->net_threshold2 = ma->net_threshold3 - 1;
        gtk_spin_button_set_value (spin_button, (gdouble) ma->net_threshold2);
    } else if (ma->net_threshold2 <= ma->net_threshold1) {
        ma->net_threshold2 = ma->net_threshold1 + 1;
        gtk_spin_button_set_value (spin_button, (gdouble) ma->net_threshold2);
    }
    g_settings_set_uint64 (ma->settings, KEY_NET_THRESHOLD2, ma->net_threshold2);
}

static void
on_net_threshold3_spin_button_value_changed (GtkSpinButton *spin_button,
                                             gpointer       user_data)
{
    MultiloadApplet *ma = user_data;
    gdouble temp;

    temp = gtk_spin_button_get_value (spin_button);
    ma->net_threshold3 = (guint64) temp;
    if (ma->net_threshold3 <= ma->net_threshold2) {
        ma->net_threshold3 = ma->net_threshold2 + 1;
        gtk_spin_button_set_value (spin_button, (gdouble) ma->net_threshold3);
    }
    g_settings_set_uint64 (ma->settings, KEY_NET_THRESHOLD3, ma->net_threshold3);
}

static void
color_button_set (GtkColorChooser *button,
                  GSettings       *settings,
                  const char      *key,
                  GdkRGBA         *color)
{
    gchar   *color_string;

    gtk_color_chooser_get_rgba (button, color);
    color_string = gdk_rgba_to_string (color);
    g_settings_set_string (settings, key, color_string);
    g_free (color_string);
}

static void
on_cpuload_usr_color_button_color_set (GtkColorButton  *button,
                                       MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_CPULOAD_USR_COLOR,
                      &(ma->graphs[graph_cpuload]->colors[cpuload_usr]));
}

static void
on_cpuload_sys_color_button_color_set (GtkColorButton  *button,
                                       MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_CPULOAD_SYS_COLOR,
                      &(ma->graphs[graph_cpuload]->colors[cpuload_sys]));
}

static void
on_cpuload_nice_color_button_color_set (GtkColorButton  *button,
                                        MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_CPULOAD_NICE_COLOR,
                      &(ma->graphs[graph_cpuload]->colors[cpuload_nice]));
}

static void
on_cpuload_iowait_color_button_color_set (GtkColorButton  *button,
                                          MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_CPULOAD_IOWAIT_COLOR,
                      &(ma->graphs[graph_cpuload]->colors[cpuload_iowait]));
}

static void
on_cpuload_free_color_button_color_set (GtkColorButton *button,
                                        MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_CPULOAD_IDLE_COLOR,
                      &(ma->graphs[graph_cpuload]->colors[cpuload_free]));
}

static void
on_memload_user_color_button_color_set (GtkColorButton  *button,
                                        MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_MEMLOAD_USER_COLOR,
                      &(ma->graphs[graph_memload]->colors[memload_user]));
}

static void
on_memload_shared_color_button_color_set (GtkColorButton  *button,
                                          MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_MEMLOAD_SHARED_COLOR,
                      &(ma->graphs[graph_memload]->colors[memload_shared]));
}

static void
on_memload_buffer_color_button_color_set (GtkColorButton  *button,
                                          MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_MEMLOAD_BUFFER_COLOR,
                      &(ma->graphs[graph_memload]->colors[memload_buffer]));
}

static void
on_memload_cached_color_button_color_set (GtkColorButton  *button,
                                          MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_MEMLOAD_CACHED_COLOR,
                      &(ma->graphs[graph_memload]->colors[memload_cached]));
}

static void
on_memload_free_color_button_color_set (GtkColorButton  *button,
                                        MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_MEMLOAD_FREE_COLOR,
                      &(ma->graphs[graph_memload]->colors[memload_free]));
}

static void
on_netload2_in_color_button_color_set (GtkColorButton  *button,
                                       MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_NETLOAD2_IN_COLOR,
                      &(ma->graphs[graph_netload2]->colors[netload2_in]));
}

static void
on_netload2_out_color_button_color_set (GtkColorButton  *button,
                                        MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_NETLOAD2_OUT_COLOR,
                      &(ma->graphs[graph_netload2]->colors[netload2_out]));
}

static void
on_netload2_loopback_color_button_color_set (GtkColorButton  *button,
                                             MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_NETLOAD2_LOOPBACK_COLOR,
                      &(ma->graphs[graph_netload2]->colors[netload2_loopback]));
}

static void
on_netload2_background_color_button_color_set (GtkColorButton  *button,
                                               MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_NETLOAD2_BACKGROUND_COLOR,
                      &(ma->graphs[graph_netload2]->colors[netload2_background]));
}

static void
on_netload2_gridline_color_button_color_set (GtkColorButton  *button,
                                             MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_NETLOAD2_GRIDLINE_COLOR,
                      &(ma->graphs[graph_netload2]->colors[netload2_gridline]));
}

static void
on_netload2_indicator_color_button_color_set (GtkColorButton  *button,
                                              MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_NETLOAD2_INDICATOR_COLOR,
                      &(ma->graphs[graph_netload2]->colors[netload2_indicator]));
}

static void
on_swapload_used_color_button_color_set (GtkColorButton  *button,
                                         MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_SWAPLOAD_USED_COLOR,
                      &(ma->graphs[graph_swapload]->colors[swapload_used]));
}

static void
on_swapload_free_color_button_color_set (GtkColorButton  *button,
                                         MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_SWAPLOAD_FREE_COLOR,
                      &(ma->graphs[graph_swapload]->colors[swapload_free]));
}

static void
on_loadavg_average_color_button_color_set (GtkColorButton  *button,
                                           MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_LOADAVG_AVERAGE_COLOR,
                      &(ma->graphs[graph_loadavg]->colors[loadavg_average]));
}

static void
on_loadavg_background_color_button_color_set (GtkColorButton  *button,
                                              MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_LOADAVG_BACKGROUND_COLOR,
                      &(ma->graphs[graph_loadavg]->colors[loadavg_background]));
}

static void
on_loadavg_gridline_color_button_color_set (GtkColorButton  *button,
                                            MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_LOADAVG_GRIDLINE_COLOR,
                      &(ma->graphs[graph_loadavg]->colors[loadavg_gridline]));
}

static void
on_diskload_read_color_button_color_set (GtkColorButton  *button,
                                         MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_DISKLOAD_READ_COLOR,
                      &(ma->graphs[graph_diskload]->colors[diskload_read]));
}

static void
on_diskload_write_color_button_color_set (GtkColorButton  *button,
                                          MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_DISKLOAD_WRITE_COLOR,
                      &(ma->graphs[graph_diskload]->colors[diskload_write]));
}

static void
on_diskload_free_color_button_color_set (GtkColorButton  *button,
                                         MultiloadApplet *ma)
{
    color_button_set (GTK_COLOR_CHOOSER (button),
                      ma->settings, KEY_DISKLOAD_FREE_COLOR,
                      &(ma->graphs[graph_diskload]->colors[diskload_free]));
}

static void
graph_set_active (MultiloadApplet *ma,
                  LoadGraph       *graph,
                  gboolean         active)
{
    graph->visible = active;
    if (active) {
        guint i;

        for (i = 0; i < graph_n; i++)
            soft_set_sensitive(ma->check_boxes[i], TRUE);
        gtk_widget_show_all (graph->main_widget);
        load_graph_start (graph);
    } else {
	load_graph_stop (graph);
        gtk_widget_hide (graph->main_widget);
        properties_set_insensitive (ma);
    }
}

#define GRAPH_ACTIVE_SET(x) graph_set_active (ma, ma->graphs[(x)], \
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox)))

static void
on_graph_cpuload_checkbox_toggled (GtkCheckButton  *checkbox,
                                   MultiloadApplet *ma)
{
    GRAPH_ACTIVE_SET (graph_cpuload);
}

static void
on_graph_memload_checkbox_toggled (GtkCheckButton  *checkbox,
                                   MultiloadApplet *ma)
{
    GRAPH_ACTIVE_SET (graph_memload);
}

static void
on_graph_netload2_checkbox_toggled (GtkCheckButton  *checkbox,
                                    MultiloadApplet *ma)
{
    GRAPH_ACTIVE_SET (graph_netload2);
}

static void
on_graph_swapload_checkbox_toggled (GtkCheckButton  *checkbox,
                                    MultiloadApplet *ma)
{
    GRAPH_ACTIVE_SET (graph_swapload);
}

static void
on_graph_loadavg_checkbox_toggled (GtkCheckButton  *checkbox,
                                   MultiloadApplet *ma)
{
    GRAPH_ACTIVE_SET (graph_loadavg);
}

static void
on_graph_diskload_checkbox_toggled (GtkCheckButton  *checkbox,
                                    MultiloadApplet *ma)
{
    GRAPH_ACTIVE_SET (graph_diskload);
}

/* save the checkbox option to gsettings and apply it on the applet */
static void
on_nvme_checkbox_toggled (GtkCheckButton  *checkbox,
                          MultiloadApplet *ma)
{
    ma->nvme_diskstats = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox));
}

static void
read_spin_uint_button (GtkWidget    *widget,
                       GSettings    *settings,
                       const char   *key,
                       guint         min,
                       guint         max)
{
    guint value;

    value = CLAMP (g_settings_get_uint (settings, key), min, max);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble) value);
    if (!g_settings_is_writable (settings, key))
        hard_set_sensitive (widget, FALSE);
}

static void
read_spin_uint64_button (GtkWidget    *widget,
                         GSettings    *settings,
                         const char   *key,
                         guint         min,
                         guint         max)
{
    guint64 value;

    value = CLAMP (g_settings_get_uint64 (settings, key), min, max);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble) value);

    if (!g_settings_is_writable (settings, key))
        hard_set_sensitive (widget, FALSE);
}

static void
read_color_button (GtkWidget    *widget,
                   GSettings    *settings,
                   const char   *key)
{
    GdkRGBA color;
    gchar *color_string;

    if ((color_string = g_settings_get_string (settings, key)) != NULL) {
        gdk_rgba_parse (&color, color_string);
        g_free (color_string);
    } else {
        gdk_rgba_parse (&color, "#000000");
    }
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (widget), &color);

    if (!g_settings_is_writable (settings, key))
        hard_set_sensitive (widget, FALSE);
}

/* show properties dialog */
void
multiload_properties_cb (GtkAction       *action,
                         MultiloadApplet *ma)
{
    GtkBuilder *builder;
    GtkWidget  *dialog = NULL;
    GtkWidget  *graph_size_spin_button_label;
    const char *graph_size_spin_button_label_txt;
    MatePanelAppletOrient orient;

    if (ma->prop_dialog) {
        dialog = ma->prop_dialog;

        gtk_window_set_screen (GTK_WINDOW (dialog),
                               gtk_widget_get_screen (GTK_WIDGET (ma->applet)));

        gtk_notebook_set_current_page (GTK_NOTEBOOK (ma->notebook),
                                       ma->last_clicked);
        gtk_window_present (GTK_WINDOW (dialog));
        return;
    }

    builder = gtk_builder_new_from_resource (MULTILOAD_RESOURCE_PATH "properties.ui");
    gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

    #define GET_WIDGET(x) (GTK_WIDGET (gtk_builder_get_object (builder, (x))))

    ma->prop_dialog = GET_WIDGET ("properties_dialog");

    read_color_button (GET_WIDGET ("cpuload_free_color_button"), ma->settings, KEY_CPULOAD_IDLE_COLOR);
    read_color_button (GET_WIDGET ("cpuload_iowait_color_button"), ma->settings, KEY_CPULOAD_IOWAIT_COLOR);
    read_color_button (GET_WIDGET ("cpuload_nice_color_button"), ma->settings, KEY_CPULOAD_NICE_COLOR);
    read_color_button (GET_WIDGET ("cpuload_sys_color_button"), ma->settings, KEY_CPULOAD_SYS_COLOR);
    read_color_button (GET_WIDGET ("cpuload_usr_color_button"), ma->settings, KEY_CPULOAD_USR_COLOR);
    read_color_button (GET_WIDGET ("diskload_free_color_button"), ma->settings, KEY_DISKLOAD_FREE_COLOR);
    read_color_button (GET_WIDGET ("diskload_read_color_button"), ma->settings, KEY_DISKLOAD_READ_COLOR);
    read_color_button (GET_WIDGET ("diskload_write_color_button"), ma->settings, KEY_DISKLOAD_WRITE_COLOR);
    read_color_button (GET_WIDGET ("loadavg_average_color_button"), ma->settings, KEY_LOADAVG_AVERAGE_COLOR);
    read_color_button (GET_WIDGET ("loadavg_background_color_button"), ma->settings, KEY_LOADAVG_BACKGROUND_COLOR);
    read_color_button (GET_WIDGET ("loadavg_gridline_color_button"), ma->settings, KEY_LOADAVG_GRIDLINE_COLOR);
    read_color_button (GET_WIDGET ("memload_buffer_color_button"), ma->settings, KEY_MEMLOAD_BUFFER_COLOR);
    read_color_button (GET_WIDGET ("memload_cached_color_button"), ma->settings, KEY_MEMLOAD_CACHED_COLOR);
    read_color_button (GET_WIDGET ("memload_free_color_button"), ma->settings, KEY_MEMLOAD_FREE_COLOR);
    read_color_button (GET_WIDGET ("memload_shared_color_button"), ma->settings, KEY_MEMLOAD_SHARED_COLOR);
    read_color_button (GET_WIDGET ("memload_user_color_button"), ma->settings, KEY_MEMLOAD_USER_COLOR);
    read_color_button (GET_WIDGET ("netload2_background_color_button"), ma->settings, KEY_NETLOAD2_BACKGROUND_COLOR);
    read_color_button (GET_WIDGET ("netload2_gridline_color_button"), ma->settings, KEY_NETLOAD2_GRIDLINE_COLOR);
    read_color_button (GET_WIDGET ("netload2_in_color_button"), ma->settings, KEY_NETLOAD2_IN_COLOR);
    read_color_button (GET_WIDGET ("netload2_indicator_color_button"), ma->settings, KEY_NETLOAD2_INDICATOR_COLOR);
    read_color_button (GET_WIDGET ("netload2_loopback_color_button"), ma->settings, KEY_NETLOAD2_LOOPBACK_COLOR);
    read_color_button (GET_WIDGET ("netload2_out_color_button"), ma->settings, KEY_NETLOAD2_OUT_COLOR);
    read_color_button (GET_WIDGET ("swapload_free_color_button"), ma->settings, KEY_SWAPLOAD_FREE_COLOR);
    read_color_button (GET_WIDGET ("swapload_used_color_button"), ma->settings, KEY_SWAPLOAD_USED_COLOR);

    graph_size_spin_button_label = GET_WIDGET ("graph_size_spin_button_label");
    orient = mate_panel_applet_get_orient(ma->applet);
    switch (orient) {
        case MATE_PANEL_APPLET_ORIENT_UP:
        case MATE_PANEL_APPLET_ORIENT_DOWN:
            graph_size_spin_button_label_txt = _("System m_onitor width:");
            break;
        default:
            graph_size_spin_button_label_txt = _("System m_onitor height:");
    }
    gtk_label_set_text_with_mnemonic (GTK_LABEL (graph_size_spin_button_label), graph_size_spin_button_label_txt);

    read_spin_uint_button (GET_WIDGET ("graph_size_spin_button"), ma->settings, GRAPH_SIZE_KEY, GRAPH_SIZE_MIN, GRAPH_SIZE_MAX);
    read_spin_uint_button (GET_WIDGET ("speed_spin_button"), ma->settings, REFRESH_RATE_KEY, REFRESH_RATE_MIN, REFRESH_RATE_MAX);

    read_spin_uint64_button (GET_WIDGET ("net_threshold1_spin_button"), ma->settings, KEY_NET_THRESHOLD1, MIN_NET_THRESHOLD1, MAX_NET_THRESHOLD1);
    read_spin_uint64_button (GET_WIDGET ("net_threshold2_spin_button"), ma->settings, KEY_NET_THRESHOLD2, MIN_NET_THRESHOLD2, MAX_NET_THRESHOLD2);
    read_spin_uint64_button (GET_WIDGET ("net_threshold3_spin_button"), ma->settings, KEY_NET_THRESHOLD3, MIN_NET_THRESHOLD3, MAX_NET_THRESHOLD3);

    ma->notebook = GET_WIDGET ("notebook");

    ma->check_boxes[graph_cpuload]  = GET_WIDGET ("graph_cpuload_checkbox");
    ma->check_boxes[graph_memload]  = GET_WIDGET ("graph_memload_checkbox");
    ma->check_boxes[graph_netload2] = GET_WIDGET ("graph_netload2_checkbox");
    ma->check_boxes[graph_swapload] = GET_WIDGET ("graph_swapload_checkbox");
    ma->check_boxes[graph_loadavg]  = GET_WIDGET ("graph_loadavg_checkbox");
    ma->check_boxes[graph_diskload] = GET_WIDGET ("graph_diskload_checkbox");

    g_settings_bind (ma->settings, VIEW_CPULOAD_KEY,  ma->check_boxes[graph_cpuload],  "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (ma->settings, VIEW_MEMLOAD_KEY,  ma->check_boxes[graph_memload],  "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (ma->settings, VIEW_NETLOAD_KEY,  ma->check_boxes[graph_netload2], "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (ma->settings, VIEW_SWAPLOAD_KEY, ma->check_boxes[graph_swapload], "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (ma->settings, VIEW_LOADAVG_KEY,  ma->check_boxes[graph_loadavg],  "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (ma->settings, VIEW_DISKLOAD_KEY, ma->check_boxes[graph_diskload], "active", G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (ma->settings, DISKLOAD_NVME_KEY, GET_WIDGET ("nvme_checkbox"), "active", G_SETTINGS_BIND_DEFAULT);

    #undef GET_WIDGET

    properties_set_insensitive (ma);

    gtk_builder_add_callback_symbols (builder,
                                      "on_cpuload_usr_color_button_color_set",         G_CALLBACK (on_cpuload_usr_color_button_color_set),
                                      "on_cpuload_sys_color_button_color_set",         G_CALLBACK (on_cpuload_sys_color_button_color_set),
                                      "on_cpuload_nice_color_button_color_set",        G_CALLBACK (on_cpuload_nice_color_button_color_set),
                                      "on_cpuload_iowait_color_button_color_set",      G_CALLBACK (on_cpuload_iowait_color_button_color_set),
                                      "on_cpuload_free_color_button_color_set",        G_CALLBACK (on_cpuload_free_color_button_color_set),
                                      "on_memload_user_color_button_color_set",        G_CALLBACK (on_memload_user_color_button_color_set),
                                      "on_memload_shared_color_button_color_set",      G_CALLBACK (on_memload_shared_color_button_color_set),
                                      "on_memload_buffer_color_button_color_set",      G_CALLBACK (on_memload_buffer_color_button_color_set),
                                      "on_memload_cached_color_button_color_set",      G_CALLBACK (on_memload_cached_color_button_color_set),
                                      "on_memload_free_color_button_color_set",        G_CALLBACK (on_memload_free_color_button_color_set),
                                      "on_netload2_in_color_button_color_set",         G_CALLBACK (on_netload2_in_color_button_color_set),
                                      "on_netload2_out_color_button_color_set",        G_CALLBACK (on_netload2_out_color_button_color_set),
                                      "on_netload2_loopback_color_button_color_set",   G_CALLBACK (on_netload2_loopback_color_button_color_set),
                                      "on_netload2_background_color_button_color_set", G_CALLBACK (on_netload2_background_color_button_color_set),
                                      "on_netload2_gridline_color_button_color_set",   G_CALLBACK (on_netload2_gridline_color_button_color_set),
                                      "on_netload2_indicator_color_button_color_set",  G_CALLBACK (on_netload2_indicator_color_button_color_set),
                                      "on_swapload_used_color_button_color_set",       G_CALLBACK (on_swapload_used_color_button_color_set),
                                      "on_swapload_free_color_button_color_set",       G_CALLBACK (on_swapload_free_color_button_color_set),
                                      "on_loadavg_average_color_button_color_set",     G_CALLBACK (on_loadavg_average_color_button_color_set),
                                      "on_loadavg_background_color_button_color_set",  G_CALLBACK (on_loadavg_background_color_button_color_set),
                                      "on_loadavg_gridline_color_button_color_set",    G_CALLBACK (on_loadavg_gridline_color_button_color_set),
                                      "on_diskload_read_color_button_color_set",       G_CALLBACK (on_diskload_read_color_button_color_set),
                                      "on_diskload_write_color_button_color_set",      G_CALLBACK (on_diskload_write_color_button_color_set),
                                      "on_diskload_free_color_button_color_set",       G_CALLBACK (on_diskload_free_color_button_color_set),
                                      "on_properties_dialog_response",                 G_CALLBACK (on_properties_dialog_response),
                                      "on_graph_cpuload_checkbox_toggled",             G_CALLBACK (on_graph_cpuload_checkbox_toggled),
                                      "on_graph_memload_checkbox_toggled",             G_CALLBACK (on_graph_memload_checkbox_toggled),
                                      "on_graph_netload2_checkbox_toggled",            G_CALLBACK (on_graph_netload2_checkbox_toggled),
                                      "on_graph_swapload_checkbox_toggled",            G_CALLBACK (on_graph_swapload_checkbox_toggled),
                                      "on_graph_loadavg_checkbox_toggled",             G_CALLBACK (on_graph_loadavg_checkbox_toggled),
                                      "on_graph_diskload_checkbox_toggled",            G_CALLBACK (on_graph_diskload_checkbox_toggled),
                                      "on_nvme_checkbox_toggled",                      G_CALLBACK (on_nvme_checkbox_toggled),
                                      "on_graph_size_spin_button_value_changed",       G_CALLBACK (on_graph_size_spin_button_value_changed),
                                      "on_speed_spin_button_value_changed",            G_CALLBACK (on_speed_spin_button_value_changed),
                                      "on_net_threshold1_spin_button_value_changed",   G_CALLBACK (on_net_threshold1_spin_button_value_changed),
                                      "on_net_threshold2_spin_button_value_changed",   G_CALLBACK (on_net_threshold2_spin_button_value_changed),
                                      "on_net_threshold3_spin_button_value_changed",   G_CALLBACK (on_net_threshold3_spin_button_value_changed),
                                      NULL);

    gtk_builder_connect_signals (builder, ma);

    g_object_unref (builder);

    gtk_window_set_screen (GTK_WINDOW (ma->prop_dialog),
                           gtk_widget_get_screen (GTK_WIDGET (ma->applet)));

    gtk_widget_show_all (ma->prop_dialog);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (ma->notebook),
                                   ma->last_clicked);
}
