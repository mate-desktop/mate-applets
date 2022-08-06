/*
 * Copyright (C) 1999 Dave Camp <dave@davec.dhs.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include "geyes.h"

#define GET_WIDGET(x) (GTK_WIDGET (gtk_builder_get_object (builder, (x))))
#define NUM_THEME_DIRECTORIES 2

static char *theme_directories[NUM_THEME_DIRECTORIES];

enum {
    COL_THEME_DIR = 0,
    COL_THEME_NAME,
    TOTAL_COLS
};

void
theme_dirs_create (void)
{
    static gboolean themes_created = FALSE;

    if (themes_created == TRUE)
        return;

    theme_directories[0] = g_build_filename (GEYES_THEMES_DIR, NULL);

    theme_directories[1] = g_build_filename (g_get_user_config_dir (), "mate",
                                             "geyes-themes", NULL);

    themes_created = TRUE;
}

static void
parse_theme_file (EyesApplet *eyes_applet,
                  FILE       *theme_file)
{
    gchar line_buf[512]; /* prolly overkill */
    gchar *token;

    if (fgets (line_buf, 512, theme_file) == NULL)
        g_debug ("fgets error");

    while (!feof (theme_file)) {
        token = strtok (line_buf, "=");
        if (strncmp (token, "wall-thickness", strlen ("wall-thickness")) == 0) {
            token += strlen ("wall-thickness");
            while (!isdigit (*token)) {
                token++;
            }
            sscanf (token, "%d", &eyes_applet->wall_thickness);
        } else if (strncmp (token, "num-eyes", strlen ("num-eyes")) == 0) {
            token += strlen ("num-eyes");
            while (!isdigit (*token)) {
                token++;
            }
            sscanf (token, "%" G_GSIZE_FORMAT, &eyes_applet->num_eyes);
            if (eyes_applet->num_eyes > MAX_EYES)
                eyes_applet->num_eyes = MAX_EYES;
        } else if (strncmp (token, "eye-pixmap", strlen ("eye-pixmap")) == 0) {
            token = strtok (NULL, "\"");
            token = strtok (NULL, "\"");
            if (eyes_applet->eye_filename != NULL)
                g_free (eyes_applet->eye_filename);
            eyes_applet->eye_filename
                = g_build_filename (eyes_applet->theme_dir, token, NULL);
        } else if (strncmp (token, "pupil-pixmap", strlen ("pupil-pixmap")) == 0) {
            token = strtok (NULL, "\"");
            token = strtok (NULL, "\"");
            if (eyes_applet->pupil_filename != NULL)
                g_free (eyes_applet->pupil_filename);
            eyes_applet->pupil_filename
                = g_build_filename (eyes_applet->theme_dir, token, NULL);
        }
        if (fgets (line_buf, 512, theme_file) == NULL)
            g_debug ("fgets error");
    }
}

int
load_theme (EyesApplet  *eyes_applet,
            const gchar *theme_dir)
{
    GtkWidget *dialog;
    FILE      *theme_file;
    gchar     *file_name;

    eyes_applet->theme_dir = g_strdup (theme_dir);

    file_name = g_build_filename (theme_dir, "config", NULL);
    if ((theme_file = fopen (file_name, "r")) == NULL) {
        g_free (eyes_applet->theme_dir);
        eyes_applet->theme_dir = g_build_filename (GEYES_THEMES_DIR, "Default-tiny", NULL);
        theme_file = fopen (GEYES_THEMES_DIR "Default-tiny/config", "r");
    }
    g_free (file_name);

    /* if it's still NULL we've got a major problem */
    if (theme_file == NULL) {
        dialog = gtk_message_dialog_new_with_markup (NULL,
                                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                     "<b>%s</b>\n\n%s",
                                                     _("Can not launch the eyes applet."),
                                                     _("There was a fatal error while trying to load the theme."));

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        gtk_widget_destroy (GTK_WIDGET (eyes_applet->applet));

        return FALSE;
    }

    parse_theme_file (eyes_applet, theme_file);
    fclose (theme_file);

    eyes_applet->theme_name = g_strdup (theme_dir);

    if (eyes_applet->eye_image)
        g_object_unref (eyes_applet->eye_image);

    eyes_applet->eye_image
        = gdk_pixbuf_new_from_file (eyes_applet->eye_filename,
                                    NULL);

    if (eyes_applet->pupil_image)
        g_object_unref (eyes_applet->pupil_image);

    eyes_applet->pupil_image
        = gdk_pixbuf_new_from_file (eyes_applet->pupil_filename,
                                    NULL);

    eyes_applet->eye_height = gdk_pixbuf_get_height (eyes_applet->eye_image);
    eyes_applet->eye_width = gdk_pixbuf_get_width (eyes_applet->eye_image);
    eyes_applet->pupil_height = gdk_pixbuf_get_height (eyes_applet->pupil_image);
    eyes_applet->pupil_width = gdk_pixbuf_get_width (eyes_applet->pupil_image);

    return TRUE;
}

static void
destroy_theme (EyesApplet *eyes_applet)
{
    /* Dunno about this - to unref or not to unref? */
    if (eyes_applet->eye_image != NULL) {
        g_object_unref (eyes_applet->eye_image);
        eyes_applet->eye_image = NULL;
    }
    if (eyes_applet->pupil_image != NULL) {
        g_object_unref (eyes_applet->pupil_image);
        eyes_applet->pupil_image = NULL;
    }

    g_free (eyes_applet->theme_dir);
    g_free (eyes_applet->theme_name);
}

static void
theme_selected_cb (GtkTreeSelection *selection,
                   gpointer          data)
{
    EyesApplet *eyes_applet = data;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *theme;

    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;

    gtk_tree_model_get (model, &iter, COL_THEME_DIR, &theme, -1);

    g_return_if_fail (theme);

    if (!g_ascii_strncasecmp (theme, eyes_applet->theme_dir,
                              strlen (theme))) {
        g_free (theme);
        return;
    }

    destroy_eyes (eyes_applet);
    destroy_theme (eyes_applet);
    load_theme (eyes_applet, theme);
    setup_eyes (eyes_applet);

    g_settings_set_string (eyes_applet->settings,
                           GEYES_SETTINGS_THEME_PATH_KEY,
                           theme);

    g_free (theme);
}

static void
phelp_cb (GtkDialog *dialog)
{
    GError *error = NULL;

    gtk_show_uri_on_window (GTK_WINDOW (dialog), "help:mate-geyes/geyes-settings",
                            gtk_get_current_event_time (),
                            &error);

    if (error) {
        GtkWidget *error_dialog
            = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_CLOSE,
                                      _("There was an error displaying help: %s"),
                                      error->message);
        g_signal_connect (error_dialog, "response",
                          G_CALLBACK (gtk_widget_destroy),
                          NULL);
        gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
        gtk_window_set_screen (GTK_WINDOW (error_dialog),
                               gtk_widget_get_screen (GTK_WIDGET (dialog)));
        gtk_widget_show (error_dialog);
        g_clear_error (&error);
    }
}

static void
presponse_cb (GtkDialog *dialog,
              gint       id,
              gpointer   data)
{
    EyesApplet *eyes_applet = data;
    if (id == GTK_RESPONSE_HELP) {
        phelp_cb (dialog);
        return;
    }

    gtk_widget_destroy (GTK_WIDGET (dialog));

    eyes_applet->prop_box.pbox = NULL;
}

void
properties_cb (GtkAction  *action,
               EyesApplet *eyes_applet)
{
    GtkBuilder *builder;
    GtkWidget *tree;
    GtkWidget *label;
    GtkListStore *model;
    GtkTreeViewColumn *column;
    GtkCellRenderer *cell;
    GtkTreeIter iter;
    GDir *dfd;
    const char *dp;
    GError *error = NULL;
    int i;

    if (eyes_applet->prop_box.pbox) {
        gtk_window_set_screen (GTK_WINDOW (eyes_applet->prop_box.pbox),
                               gtk_widget_get_screen (GTK_WIDGET (eyes_applet->applet)));

        gtk_window_present (GTK_WINDOW (eyes_applet->prop_box.pbox));
        return;
    }

    builder = gtk_builder_new_from_resource (GRESOURCE_PREFIX "/eyes/themes.ui");

    eyes_applet->prop_box.pbox = GET_WIDGET("preferences_dialog");
    tree = GET_WIDGET("themes_treeview");
    label = GET_WIDGET("select_theme_label");

    model = gtk_list_store_new (TOTAL_COLS, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (model));
    cell = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("not used", cell, "text",
                                                       COL_THEME_NAME,
                                                       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    if (!g_settings_is_writable (eyes_applet->settings, GEYES_SETTINGS_THEME_PATH_KEY)) {
        gtk_widget_set_sensitive (tree, FALSE);
        gtk_widget_set_sensitive (label, FALSE);
    }

    for (i = 0; i < NUM_THEME_DIRECTORIES; i++) {
        dfd = g_dir_open (theme_directories[i], 0, &error);
        if (error) {
            g_debug ("Could not open the folder: %s", error->message);
            g_clear_error (&error);
            continue;
        }
        while ((dp = g_dir_read_name (dfd)) != NULL) {
            if (dp[0] != '.') {
                gchar *theme_dir;

                theme_dir = g_build_filename (theme_directories[i], dp, NULL);
                gtk_list_store_append (model, &iter);
                gtk_list_store_set (model, &iter,
                                    COL_THEME_DIR, theme_dir,
                                    COL_THEME_NAME, dp,
                                    -1);

                if (!g_ascii_strncasecmp (eyes_applet->theme_dir, theme_dir,
                                          strlen (theme_dir))) {
                    GtkTreePath *path;

                    path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
                    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree),
                                              path, NULL, FALSE);
                    gtk_tree_path_free (path);
                }
                g_free (theme_dir);
            }
        }
        g_dir_close (dfd);
    }
    g_object_unref (model);

    /* signals */
    gtk_builder_add_callback_symbols (builder, "on_preferences_dialog_response",
                                      G_CALLBACK (presponse_cb),
                                      "on_themes_treeselection_changed",
                                      G_CALLBACK (theme_selected_cb),
                                      NULL);
    gtk_builder_connect_signals (builder, eyes_applet);

    g_object_unref (builder);

    gtk_widget_show_all (eyes_applet->prop_box.pbox);

    return;
}
