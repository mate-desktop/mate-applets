/* timerapplet.c:
 *
 * Copyright (C) 2014 Stefano Karapetsas
 *
 * This file is part of MATE Applets.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *      Stefano Karapetsas <stefano@karapetsas.com>
 */

#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <libnotify/notify.h>

#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

/* Applet constants */
#define APPLET_ICON  "mate-panel-clock"
#define STEP         100

/* GSettings constants */
#define TIMER_SCHEMA            "org.mate.panel.applet.timer"
#define NAME_KEY                "name"
#define DURATION_KEY            "duration"
#define SHOW_NOTIFICATION_KEY   "show-notification"
#define SHOW_DIALOG_KEY         "show-dialog"

#define GET_WIDGET(x) (GTK_WIDGET (gtk_builder_get_object (builder, (x))))
#define GET_DIALOG(x) (GTK_DIALOG (gtk_builder_get_object (builder, (x))))
#define GET_SPIN_BUTTON(x) (GTK_SPIN_BUTTON (gtk_builder_get_object (builder, (x))))

typedef struct
{
    MatePanelApplet   *applet;

    GSettings         *settings;

    GtkActionGroup    *action_group;
    GtkLabel          *label;
    GtkImage          *image;
    GtkImage          *pause_image;
    GtkBox            *box;

    GtkSpinButton     *hours;
    GtkSpinButton     *minutes;
    GtkSpinButton     *seconds;

    gboolean           active;
    gboolean           pause;
    gint               elapsed;

    guint              timeout_id;
} TimerApplet;

static void timer_start_callback (GtkAction *action, TimerApplet *applet);
static void timer_pause_callback (GtkAction *action, TimerApplet *applet);
static void timer_stop_callback (GtkAction *action, TimerApplet *applet);
static void timer_about_callback (GtkAction *action, TimerApplet *applet);
static void timer_reset_callback (GtkAction *action, TimerApplet *applet);
static void timer_preferences_callback (GtkAction *action, TimerApplet *applet);

static const GtkActionEntry applet_menu_actions [] = {
    { "Start", "media-playback-start", N_("_Start timer"), NULL, NULL, G_CALLBACK (timer_start_callback) },
    { "Pause", "media-playback-pause", N_("P_ause timer"), NULL, NULL, G_CALLBACK (timer_pause_callback) },
    { "Stop", "media-playback-stop", N_("S_top timer"), NULL, NULL, G_CALLBACK (timer_stop_callback) },
    { "Reset", "edit-undo", N_("R_eset"), NULL, NULL, G_CALLBACK (timer_reset_callback) },
    { "Preferences", "document-properties", N_("_Preferences"), NULL, NULL, G_CALLBACK (timer_preferences_callback) },
    { "About", "help-about", N_("_About"), NULL, NULL, G_CALLBACK (timer_about_callback) }
};

static char *ui = "<menuitem name='Item 1' action='Start' />"
                  "<menuitem name='Item 2' action='Pause' />"
                  "<menuitem name='Item 3' action='Stop' />"
                  "<menuitem name='Item 4' action='Reset' />"
                  "<menuitem name='Item 5' action='Preferences' />"
                  "<menuitem name='Item 6' action='About' />";

static void
timer_applet_destroy (MatePanelApplet *applet_widget, TimerApplet *applet)
{
    g_assert (applet);

    if (applet->timeout_id != 0)
    {
        g_source_remove(applet->timeout_id);
        applet->timeout_id = 0;
    }

    g_object_unref (applet->settings);

    notify_uninit ();
}

/* timer management */
static gboolean
timer_callback (TimerApplet *applet)
{
    AtkObject *atk_obj;
    gboolean retval = TRUE;
    gchar *label;
    gchar *name;
    gchar *tooltip;
    gint hours, minutes, seconds, duration, remaining;

    label = NULL;
    tooltip = NULL;

    if (!GTK_IS_WIDGET (applet->label))
        return FALSE;

    name = g_settings_get_string (applet->settings, NAME_KEY);
    atk_obj = gtk_widget_get_accessible (GTK_WIDGET (applet->applet));

    if (!applet->active)
    {
        applet->pause = FALSE;
        applet->elapsed = 0;

        gtk_label_set_text (applet->label, name);
        gtk_widget_set_tooltip_text (GTK_WIDGET (applet->label), "");
        gtk_widget_hide (GTK_WIDGET (applet->pause_image));
        atk_object_set_name (atk_obj, name);
    }
    else
    {
        if (!applet->pause)
            applet->elapsed += STEP;

        duration = g_settings_get_int (applet->settings, DURATION_KEY);

        remaining = duration - (applet->elapsed / 1000);

        if (remaining <= 0)
        {
            applet->active = FALSE;
            applet->timeout_id = 0;

            /* Translators: %s is a placeholder for the timer name, 'Timer' by default */
            label = g_strdup_printf (_("Finished %s"), name);
            gtk_label_set_text (applet->label, label);
            gtk_widget_set_tooltip_text (GTK_WIDGET (applet->label), name);
            gtk_widget_hide (GTK_WIDGET (applet->pause_image));
            atk_object_set_name (atk_obj, label);
            atk_object_set_description (atk_obj, "");

            if (g_settings_get_boolean (applet->settings, SHOW_NOTIFICATION_KEY))
            {
                NotifyNotification *n;
                n = notify_notification_new (name, _("Timer finished!"), APPLET_ICON);
                notify_notification_set_timeout (n, 30000);
                notify_notification_show (n, NULL);
                g_object_unref (G_OBJECT (n));
            }

            if (g_settings_get_boolean (applet->settings, SHOW_DIALOG_KEY))
            {
                GtkWidget *dialog = gtk_message_dialog_new_with_markup (NULL,
                                                                        GTK_DIALOG_MODAL,
                                                                        GTK_MESSAGE_INFO,
                                                                        GTK_BUTTONS_OK,
                                                                        "<b>%s</b>\n\n%s", name, _("Timer finished!"));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
            }

            /* stop further calls */
            retval = FALSE;
        }
        else
        {
            hours = remaining / 60 / 60;
            minutes = remaining / 60 % 60;
            seconds = remaining % 60;

            if (hours > 0)
                label = g_strdup_printf ("%02d:%02d:%02d", hours, minutes, seconds);
            else
                label = g_strdup_printf ("%02d:%02d", minutes, seconds);

            hours = duration / 60 / 60;
            minutes = duration / 60 % 60;
            seconds = duration % 60;

            if (hours > 0)
                tooltip = g_strdup_printf ("%s (%02d:%02d:%02d)", name, hours, minutes, seconds);
            else
                tooltip = g_strdup_printf ("%s (%02d:%02d)", name, minutes, seconds);

            gtk_label_set_text (applet->label, label);
            gtk_widget_set_tooltip_text (GTK_WIDGET (applet->label), tooltip);
            gtk_widget_set_visible (GTK_WIDGET (applet->pause_image), applet->pause);
            atk_object_set_name (atk_obj, label);
        }

        g_free (label);
        g_free (tooltip);
    }

    /* update actions sensitiveness */
    gtk_action_set_sensitive (gtk_action_group_get_action (applet->action_group, "Start"), !applet->active || applet->pause);
    gtk_action_set_sensitive (gtk_action_group_get_action (applet->action_group, "Pause"), applet->active && !applet->pause);
    gtk_action_set_sensitive (gtk_action_group_get_action (applet->action_group, "Stop"), applet->active);
    gtk_action_set_sensitive (gtk_action_group_get_action (applet->action_group, "Reset"), !applet->active && !applet->pause && applet->elapsed);
    gtk_action_set_sensitive (gtk_action_group_get_action (applet->action_group, "Preferences"), !applet->active && !applet->pause);

    g_free (name);

    return retval;
}

/* start action */
static void
timer_start_callback (GtkAction *action, TimerApplet *applet)
{
    applet->active = TRUE;
    if (applet->pause)
        applet->pause = FALSE;
    else
        applet->elapsed = 0;
    applet->timeout_id = g_timeout_add (STEP, (GSourceFunc) timer_callback, applet);
}

/* pause action */
static void
timer_pause_callback (GtkAction *action, TimerApplet *applet)
{
    applet->pause = TRUE;
    if (applet->timeout_id != 0)
    {
        g_source_remove(applet->timeout_id);
        applet->timeout_id = 0;
    }
    timer_callback (applet);
}

/* stop action */
static void
timer_stop_callback (GtkAction *action, TimerApplet *applet)
{
    applet->active = FALSE;
    if (applet->timeout_id != 0)
    {
        g_source_remove(applet->timeout_id);
        applet->timeout_id = 0;
    }
    timer_callback (applet);
}

/* reset action */
static void
timer_reset_callback (GtkAction *action, TimerApplet *applet)
{
    applet->active = FALSE;
    applet->pause = FALSE;
    applet->elapsed = 0;
    timer_callback (applet);
}

/* Show the about dialog */
static void
timer_about_callback (GtkAction *action, TimerApplet *applet)
{
    const char* authors[] = { "Stefano Karapetsas <stefano@karapetsas.com>", NULL };

    gtk_show_about_dialog(NULL,
                          "title", _("About Timer Applet"),
                          "version", VERSION,
                          "copyright", _("Copyright \xc2\xa9 2014 Stefano Karapetsas\n"
                                         "Copyright \xc2\xa9 2015-2021 MATE developers"),
                          "authors", authors,
                          "comments", _("Start a timer and receive a notification when it is finished"),
                          "translator-credits", _("translator-credits"),
                          "logo-icon-name", APPLET_ICON,
                          NULL);
}

/* calculate duration and save in GSettings */
static void
timer_spin_button_value_changed (GtkSpinButton *spinbutton, TimerApplet *applet)
{
    gint duration;

    duration =  gtk_spin_button_get_value_as_int (applet->seconds);
    duration += gtk_spin_button_get_value_as_int (applet->minutes) * 60;
    duration += gtk_spin_button_get_value_as_int (applet->hours)   * 3600;

    g_settings_set_int (applet->settings, DURATION_KEY, duration);
}

/* Show the preferences dialog */
static void
timer_preferences_callback (GtkAction *action, TimerApplet *applet)
{
    GtkBuilder *builder;
    GtkDialog *dialog;
    gint duration, hours, minutes, seconds;

    builder = gtk_builder_new_from_resource ("/org/mate/mate-applets/timerapplet/timerapplet-preferences.ui");

    dialog = GET_DIALOG ("preferences_dialog");
    applet->hours = GET_SPIN_BUTTON ("hours_spinbutton");
    applet->minutes = GET_SPIN_BUTTON ("minutes_spinbutton");
    applet->seconds = GET_SPIN_BUTTON ("seconds_spinbutton");

    duration = g_settings_get_int (applet->settings, DURATION_KEY);
    hours = duration / 60 / 60;
    minutes = duration / 60 % 60;
    seconds = duration % 60;

    gtk_spin_button_set_value (applet->hours,   hours);
    gtk_spin_button_set_value (applet->minutes, minutes);
    gtk_spin_button_set_value (applet->seconds, seconds);

    g_settings_bind (applet->settings, NAME_KEY, GET_WIDGET ("name_entry"), "text", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (applet->settings, SHOW_NOTIFICATION_KEY, GET_WIDGET ("show_notification_popup_check"), "active", G_SETTINGS_BIND_DEFAULT);
    g_settings_bind (applet->settings, SHOW_DIALOG_KEY, GET_WIDGET ("show_dialog_check"), "active", G_SETTINGS_BIND_DEFAULT);

    /* signals */
    gtk_builder_add_callback_symbols (builder,
                                      "on_hours_spinbutton_value_changed", G_CALLBACK (timer_spin_button_value_changed),
                                      "on_minutes_spinbutton_value_changed", G_CALLBACK (timer_spin_button_value_changed),
                                      "on_seconds_spinbutton_value_changed", G_CALLBACK (timer_spin_button_value_changed),
                                      NULL);
    gtk_builder_connect_signals (builder, applet);

    g_signal_connect (dialog, "response",
                      G_CALLBACK (gtk_widget_destroy),
                      dialog);

    g_object_unref (builder);

    gtk_widget_show_all (GTK_WIDGET (dialog));
}

static gboolean
timer_applet_click (TimerApplet *applet,  GdkEventButton *event)
{
    if (  event->button == 1)
    {
        if (!applet->active && !applet->pause && applet->elapsed)
            timer_reset_callback (NULL, applet);
        else if (applet->active && !applet->pause)
            timer_pause_callback (NULL, applet);
        else if (!applet->active || applet->pause)
            timer_start_callback (NULL, applet);
    return TRUE;
    }
    else
        return FALSE;
}

static void
timer_settings_changed (GSettings *settings, gchar *key, TimerApplet *applet)
{
    timer_callback (applet);
}

static gboolean
timer_applet_fill (MatePanelApplet* applet_widget)
{
    TimerApplet *applet;
    AtkObject   *atk_obj;

#ifndef ENABLE_IN_PROCESS
    g_set_application_name (_("Timer Applet"));
#endif
    gtk_window_set_default_icon_name (APPLET_ICON);

    if (!notify_is_initted ())
        notify_init ("timer-applet");

    mate_panel_applet_set_flags (applet_widget, MATE_PANEL_APPLET_EXPAND_MINOR);

    applet = g_malloc0(sizeof(TimerApplet));
    applet->applet = applet_widget;
    applet->settings = mate_panel_applet_settings_new (applet_widget,TIMER_SCHEMA);
    applet->timeout_id = 0;
    applet->active = FALSE;
    applet->pause = FALSE;

    applet->box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    applet->image = GTK_IMAGE (gtk_image_new_from_icon_name (APPLET_ICON, GTK_ICON_SIZE_BUTTON));
    applet->pause_image = GTK_IMAGE (gtk_image_new_from_icon_name ("media-playback-pause", GTK_ICON_SIZE_BUTTON));
    applet->label = GTK_LABEL (gtk_label_new (""));

    atk_obj = gtk_widget_get_accessible (GTK_WIDGET (applet->applet));
    if (GTK_IS_ACCESSIBLE (atk_obj)) {
        atk_object_set_name (atk_obj, _("Timer Applet"));
        atk_object_set_description (atk_obj, _("Start a timer and receive a notification when it is finished"));
    }
    /* we add the Gtk label into the applet */
    gtk_box_pack_start (applet->box,
                        GTK_WIDGET (applet->image),
                        TRUE, TRUE, 0);
    gtk_box_pack_start (applet->box,
                        GTK_WIDGET (applet->pause_image),
                        TRUE, TRUE, 0);
    gtk_box_pack_start (applet->box,
                        GTK_WIDGET (applet->label),
                        TRUE, TRUE, 3);

    gtk_container_add (GTK_CONTAINER (applet_widget),
                       GTK_WIDGET (applet->box));

    gtk_widget_show_all (GTK_WIDGET (applet->applet));
    gtk_widget_hide (GTK_WIDGET (applet->pause_image));

    g_signal_connect (applet->applet, "destroy",
                      G_CALLBACK (timer_applet_destroy),
                      applet);

    g_signal_connect_swapped (applet->applet, "button_press_event",
                              G_CALLBACK (timer_applet_click),
                              applet);

    /* set up context menu */
    applet->action_group = gtk_action_group_new ("Timer Applet Actions");
    gtk_action_group_set_translation_domain (applet->action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (applet->action_group, applet_menu_actions,
                                  G_N_ELEMENTS (applet_menu_actions), applet);
    mate_panel_applet_setup_menu (applet->applet, ui, applet->action_group);

    /* execute callback to set actions sensitiveness */
    timer_callback (applet);

    /* GSettings callback */
    g_signal_connect (applet->settings, "changed",
                      G_CALLBACK (timer_settings_changed),
                      applet);

    return TRUE;
}

/* this function, called by mate-panel, will create the applet */
static gboolean
timer_factory (MatePanelApplet* applet, const char* iid, gpointer data)
{
    gboolean retval = FALSE;

    if (!g_strcmp0 (iid, "TimerApplet"))
        retval = timer_applet_fill (applet);

    return retval;
}

/* needed by mate-panel applet library */
PANEL_APPLET_FACTORY ("TimerAppletFactory",
                      PANEL_TYPE_APPLET,
                      "Timer applet",
                      timer_factory,
                      NULL)
