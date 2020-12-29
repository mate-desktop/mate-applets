/* command.c:
 *
 * Copyright (C) 2013-2014 Stefano Karapetsas
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
#include <gmodule.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>
#include "ma-command.h"

/* Applet constants */
#define APPLET_ICON    "utilities-terminal"
#define ERROR_OUTPUT   "#"

/* GSettings constants */
#define COMMAND_SCHEMA "org.mate.panel.applet.command"
#define COMMAND_KEY    "command"
#define INTERVAL_KEY   "interval"
#define SHOW_ICON_KEY  "show-icon"
#define WIDTH_KEY      "width"

/* GKeyFile constants */
#define GK_COMMAND_GROUP   "Command"
#define GK_COMMAND_OUTPUT  "Output"
#define GK_COMMAND_ICON    "Icon"

#define GET_WIDGET(x) (GTK_WIDGET (gtk_builder_get_object (builder, (x))))
#define GET_DIALOG(x) (GTK_DIALOG (gtk_builder_get_object (builder, (x))))

typedef struct
{
    MatePanelApplet   *applet;

    GSettings         *settings;

    GtkLabel          *label;
    GtkImage          *image;
    GtkBox            *box;
    MaCommand         *command;
    GCancellable      *cancellable;
    gboolean           running;

    gchar             *cmdline;
    gint               interval;
    gint               width;

    guint              timeout_id;
} CommandApplet;

static void command_about_callback (GtkAction *action, CommandApplet *command_applet);
static void command_settings_callback (GtkAction *action, CommandApplet *command_applet);
static gboolean command_execute (CommandApplet *command_applet);
static gboolean command_text_changed (GtkWidget *widget, GdkEvent  *event, gpointer user_data);
static void interval_value_changed (GtkSpinButton *spin_button, gpointer user_data);
static void width_value_changed (GtkSpinButton *spin_button, gpointer user_data);
static void command_async_ready_callback (GObject *source_object, GAsyncResult *res, gpointer user_data);
static gboolean timeout_callback (CommandApplet *command_applet);

static const GtkActionEntry applet_menu_actions [] = {
    { "Preferences", "document-properties", N_("_Preferences"), NULL, NULL, G_CALLBACK (command_settings_callback) },
    { "About", "help-about", N_("_About"), NULL, NULL, G_CALLBACK (command_about_callback) }
};

static char *ui = "<menuitem name='Item 1' action='Preferences' />"
                  "<menuitem name='Item 2' action='About' />";

static void
command_applet_destroy (MatePanelApplet *applet_widget, CommandApplet *command_applet)
{
    g_assert (command_applet);

    if (command_applet->timeout_id != 0)
    {
        g_source_remove (command_applet->timeout_id);
        command_applet->timeout_id = 0;
    }

    if (command_applet->cmdline != NULL)
    {
        g_free (command_applet->cmdline);
        command_applet->cmdline = NULL;
    }

    if (command_applet->command != NULL)
    {
        g_object_unref (command_applet->command);
    }

    g_object_unref (command_applet->settings);
}

/* Show the about dialog */
static void
command_about_callback (GtkAction *action, CommandApplet *command_applet)
{
    const char* authors[] = { "Stefano Karapetsas <stefano@karapetsas.com>", NULL };

    gtk_show_about_dialog(NULL,
                          "title", _("About Command Applet"),
                          "version", VERSION,
                          "copyright", _("Copyright \xc2\xa9 2013-2014 Stefano Karapetsas\n"
                                         "Copyright \xc2\xa9 2015-2020 MATE developers"),
                          "authors", authors,
                          "comments", _("Shows the output of a command"),
                          "translator-credits", _("translator-credits"),
                          "logo-icon-name", APPLET_ICON,
                          NULL );
}

static gboolean
command_text_changed (GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
    const gchar *text;
    CommandApplet *command_applet;

    command_applet = (CommandApplet*) user_data;
    text = gtk_entry_get_text (GTK_ENTRY(widget));
    if (g_strcmp0(command_applet->cmdline, text) == 0) {
        return TRUE;
    }

    if (strlen (text) == 0) {
        gtk_label_set_text (command_applet->label, ERROR_OUTPUT);
        return TRUE;
    }

    g_settings_set_string (command_applet->settings, COMMAND_KEY, text);
    return TRUE;
}

static void interval_value_changed (GtkSpinButton *spin_button, gpointer user_data)
{
    gint value;
    CommandApplet *command_applet;

    command_applet = (CommandApplet*) user_data;
    value = gtk_spin_button_get_value_as_int (spin_button);
    if (command_applet->interval == value) {
        return;
    }

    g_settings_set_int (command_applet->settings, INTERVAL_KEY, value);
}

static void width_value_changed (GtkSpinButton *spin_button, gpointer user_data)
{
    gint value;
    CommandApplet *command_applet;

    command_applet = (CommandApplet*) user_data;
    value = gtk_spin_button_get_value_as_int (spin_button);
    if (command_applet->width == value) {
        return;
    }

    g_settings_set_int (command_applet->settings, WIDTH_KEY, value);
}

/* Show the preferences dialog */
static void
command_settings_callback (GtkAction *action, CommandApplet *command_applet)
{
    GtkDialog *dialog;
    GtkBuilder *builder;

    builder = gtk_builder_new_from_resource ("/org/mate/mate-applets/command/command-preferences.ui");

    dialog = GET_DIALOG ("preferences_dialog");

    g_settings_bind (command_applet->settings, COMMAND_KEY, GET_WIDGET ("command_entry"), "text", G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (command_applet->settings, INTERVAL_KEY, GET_WIDGET ("interval_spinbutton"), "value", G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (command_applet->settings, WIDTH_KEY, GET_WIDGET ("width_spinbutton"), "value", G_SETTINGS_BIND_GET_NO_CHANGES);
    g_settings_bind (command_applet->settings, SHOW_ICON_KEY, GET_WIDGET ("show_icon_check"), "active", G_SETTINGS_BIND_DEFAULT);

    /* signals */
    gtk_builder_add_callback_symbols (builder,
                                      "on_command_entry_focus_out_event", G_CALLBACK (command_text_changed),
                                      "on_interval_spinbutton_value_changed", G_CALLBACK (interval_value_changed),
                                      "on_width_spinbutton_value_changed", G_CALLBACK (width_value_changed),
                                      NULL);
    gtk_builder_connect_signals (builder, command_applet);

    g_signal_connect (dialog, "response",
                      G_CALLBACK (gtk_widget_destroy),
                      dialog);

    g_object_unref (builder);

    gtk_widget_show_all (GTK_WIDGET (dialog));
}

/* GSettings signal callbacks */
static void
settings_command_changed (GSettings *settings, gchar *key, CommandApplet *command_applet)
{
    GError *error = NULL;
    gchar *cmdline;
    gchar **argv;

    cmdline = g_settings_get_string (command_applet->settings, COMMAND_KEY);
    if (strlen (cmdline) == 0 || g_strcmp0(command_applet->cmdline, cmdline) == 0)
        return;

    if (!g_shell_parse_argv (cmdline, NULL, &argv, &error))
    {
        gtk_label_set_text (command_applet->label, ERROR_OUTPUT);
        g_clear_error (&error);
        return;
    }
    g_strfreev(argv);

    if (command_applet->cmdline)
        g_free (command_applet->cmdline);
    command_applet->cmdline = cmdline;

    command_execute (command_applet);
}

static void
settings_width_changed (GSettings *settings, gchar *key, CommandApplet *command_applet)
{
    gint width;

    width = g_settings_get_int (command_applet->settings, WIDTH_KEY);

    if (command_applet->width != width) {
        command_applet->width = width;
    }
}

static void
settings_interval_changed (GSettings *settings, gchar *key, CommandApplet *command_applet)
{
    gint interval;

    interval = g_settings_get_int (command_applet->settings, INTERVAL_KEY);

    /* minimum interval */
    if (interval < 1)
        interval = 1;

    if (command_applet->interval == interval) {
        return;
    }
    command_applet->interval = interval;

    command_execute (command_applet);
}

static void
process_command_output (CommandApplet *command_applet, gchar *output)
{
    gtk_widget_set_tooltip_text (GTK_WIDGET (command_applet->label), command_applet->cmdline);

    if ((output == NULL) || (output[0] == '\0'))
    {
        gtk_label_set_text (command_applet->label, ERROR_OUTPUT);
        return;
    }

    /* check if output is a custom GKeyFile */
    if (g_str_has_prefix (output, "[Command]"))
    {
        GKeyFile *file = g_key_file_new ();
        if (g_key_file_load_from_data (file, output, -1, G_KEY_FILE_NONE, NULL))
        {
            gchar *goutput = g_key_file_get_string (file, GK_COMMAND_GROUP, GK_COMMAND_OUTPUT, NULL);
            gchar *icon = g_key_file_get_string (file, GK_COMMAND_GROUP, GK_COMMAND_ICON, NULL);

            if (goutput)
            {
                gtk_label_set_use_markup (command_applet->label, TRUE);
                gtk_label_set_markup (command_applet->label, goutput);
            }

            if (icon)
                gtk_image_set_from_icon_name (command_applet->image, icon, GTK_ICON_SIZE_LARGE_TOOLBAR);

            g_free (goutput);
            g_free (icon);
        }
        else
            gtk_label_set_text (command_applet->label, ERROR_OUTPUT);

        g_key_file_free (file);
    }
    else
    {
        /* Remove leading and trailing whitespace */
        g_strstrip (output);

        /* check output length */
        if (g_utf8_strlen (output, strlen(output)) > command_applet->width)
        {
            *g_utf8_offset_to_pointer(output, command_applet->width) = '\0';
        }

        gtk_label_set_text (command_applet->label, output);
    }
}

static void command_async_ready_callback (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    gchar *output;
    GError *error = NULL;
    CommandApplet *command_applet;

    command_applet = (CommandApplet*) user_data;

    output = ma_command_run_finish (command_applet->command, res, &error);
    if (error == NULL) {
        process_command_output (command_applet, output);
    } else {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_FAILED)) {
            gtk_label_set_text (command_applet->label, ERROR_OUTPUT);
        }
        g_error_free (error);
    }
    g_free (output);
    command_applet->running = FALSE;
}

static gboolean timeout_callback (CommandApplet *command_applet)
{
    /* command is empty, wait for next timer execution */
    if (strlen (command_applet->cmdline) == 0) {
        return G_SOURCE_CONTINUE;
    }

    /* command running, wait for next timer execution */
    if (command_applet->running) {
        return G_SOURCE_CONTINUE;
    } else {
        gchar **argv;
        GError *error = NULL;
        if (!g_shell_parse_argv (command_applet->cmdline, NULL, &argv, &error)) {
            gtk_label_set_text (command_applet->label, ERROR_OUTPUT);
            g_clear_error (&error);
            return G_SOURCE_CONTINUE;
        }
        g_strfreev(argv);
        command_execute (command_applet);
        return G_SOURCE_REMOVE;
    }
}

static gboolean
command_execute (CommandApplet *command_applet)
{
    /* stop current timer */
    if (command_applet->timeout_id != 0)
    {
        g_source_remove (command_applet->timeout_id);
        command_applet->timeout_id = 0;
    }

    if (command_applet->running) {
        g_cancellable_cancel (command_applet->cancellable);
    }

    g_object_set (G_OBJECT(command_applet->command), "command", command_applet->cmdline, NULL);
    ma_command_run_async (command_applet->command,
                          command_applet->cancellable,
                          command_async_ready_callback,
                          command_applet);
    if (!command_applet->running) {
        command_applet->running = TRUE;
    }

    if (g_cancellable_is_cancelled (command_applet->cancellable)) {
        g_cancellable_reset (command_applet->cancellable);
    }

    command_applet->timeout_id = g_timeout_add_seconds (command_applet->interval,
                                                        (GSourceFunc) timeout_callback,
                                                        command_applet);
    return G_SOURCE_CONTINUE;
}

static gboolean
command_applet_fill (MatePanelApplet* applet)
{
    CommandApplet *command_applet;

    g_set_application_name (_("Command Applet"));
    gtk_window_set_default_icon_name (APPLET_ICON);

    mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);

    command_applet = g_malloc0(sizeof(CommandApplet));
    command_applet->applet = applet;
    command_applet->settings = mate_panel_applet_settings_new (applet, COMMAND_SCHEMA);

    command_applet->interval = g_settings_get_int (command_applet->settings, INTERVAL_KEY);
    command_applet->cmdline = g_settings_get_string (command_applet->settings, COMMAND_KEY);
    command_applet->width = g_settings_get_int (command_applet->settings, WIDTH_KEY);
    command_applet->command = ma_command_new(command_applet->cmdline, NULL);
    command_applet->cancellable = g_cancellable_new ();

    command_applet->box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
    command_applet->image = GTK_IMAGE (gtk_image_new_from_icon_name (APPLET_ICON, GTK_ICON_SIZE_LARGE_TOOLBAR));
    command_applet->label = GTK_LABEL (gtk_label_new (ERROR_OUTPUT));
    command_applet->timeout_id = 0;

    /* we add the Gtk label into the applet */
    gtk_box_pack_start (command_applet->box,
                        GTK_WIDGET (command_applet->image),
                        TRUE, TRUE, 0);
    gtk_box_pack_start (command_applet->box,
                        GTK_WIDGET (command_applet->label),
                        TRUE, TRUE, 0);

    gtk_container_add (GTK_CONTAINER (applet),
                       GTK_WIDGET (command_applet->box));

    gtk_widget_show_all (GTK_WIDGET (command_applet->applet));

    g_signal_connect (command_applet->applet, "destroy",
                      G_CALLBACK (command_applet_destroy),
                      command_applet);

    /* GSettings signals */
    g_signal_connect (command_applet->settings, "changed::" COMMAND_KEY,
                      G_CALLBACK (settings_command_changed),
                      command_applet);

    g_signal_connect (command_applet->settings, "changed::" INTERVAL_KEY,
                      G_CALLBACK (settings_interval_changed),
                      command_applet);

    g_signal_connect (command_applet->settings, "changed::" WIDTH_KEY,
                      G_CALLBACK (settings_width_changed),
                      command_applet);

    g_settings_bind (command_applet->settings,
                     SHOW_ICON_KEY,
                     command_applet->image,
                     "visible",
                     G_SETTINGS_BIND_DEFAULT);

    /* set up context menu */
    GtkActionGroup *action_group = gtk_action_group_new ("Command Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group, applet_menu_actions,
                                  G_N_ELEMENTS (applet_menu_actions), command_applet);
    mate_panel_applet_setup_menu (command_applet->applet, ui, action_group);

    /* first command execution */
    command_execute (command_applet);
    return TRUE;
}

/* this function, called by mate-panel, will create the applet */
static gboolean
command_factory (MatePanelApplet* applet, const char* iid, gpointer data)
{
    gboolean retval = FALSE;

    if (!g_strcmp0 (iid, "CommandApplet"))
        retval = command_applet_fill (applet);

    return retval;
}

/* needed by mate-panel applet library */
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY("CommandAppletFactory",
                                      PANEL_TYPE_APPLET,
                                      "Command applet",
                                      command_factory,
                                      NULL)
