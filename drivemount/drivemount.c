/* -*- mode: C; c-basic-offset: 4 -*-
 * Drive Mount Applet
 * Copyright (c) 2004 Canonical Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
 * Author:
 *   James Henstridge <jamesh@canonical.com>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>

#include "drive-list.h"

static const char drivemount_iid[] = "DriveMountApplet";
static const char factory_iid[] = "DriveMountAppletFactory";

static void
change_orient (MatePanelApplet *applet, MatePanelAppletOrient o, DriveList *drive_list)
{
    GtkOrientation orientation;

    switch (o) {
    case MATE_PANEL_APPLET_ORIENT_LEFT:
    case MATE_PANEL_APPLET_ORIENT_RIGHT:
	orientation = GTK_ORIENTATION_VERTICAL;
	break;
    case MATE_PANEL_APPLET_ORIENT_UP:
    case MATE_PANEL_APPLET_ORIENT_DOWN:
    default:
	orientation = GTK_ORIENTATION_HORIZONTAL;
	break;
    }
    drive_list_set_orientation (drive_list, orientation);
}

static void
size_allocate (MatePanelApplet  *applet,
	       GdkRectangle *allocation,
	       DriveList    *drive_list)
{
    int size;

    switch (mate_panel_applet_get_orient (applet)) {
    case MATE_PANEL_APPLET_ORIENT_LEFT:
    case MATE_PANEL_APPLET_ORIENT_RIGHT:
	size = allocation->width;
	break;
    case MATE_PANEL_APPLET_ORIENT_UP:
    case MATE_PANEL_APPLET_ORIENT_DOWN:
    default:
	size = allocation->height;
	break;
    }
    drive_list_set_panel_size (drive_list, size);
}

static void
change_background (MatePanelApplet               *applet,
		   MatePanelAppletBackgroundType  type,
		   GdkColor                  *colour,
		   cairo_pattern_t           *pattern,
		   DriveList                 *drivelist)
{
    switch (type) {
    case PANEL_NO_BACKGROUND:
	drive_list_set_transparent (drivelist, FALSE);
	break;

    case PANEL_COLOR_BACKGROUND:
    case PANEL_PIXMAP_BACKGROUND:
	drive_list_set_transparent (drivelist, TRUE);
	break;
    }
}

static void
display_about_dialog (GtkAction *action,
		      DriveList *drive_list)
{
    const gchar *authors[] = {
	"James Henstridge <jamesh@canonical.com>",
	NULL
    };
    char copyright[] = \
	"Copyright \xc2\xa9 2012-2018 MATE developers\n"
	"Copyright \xC2\xA9 2004 Canonical Ltd";

    const gchar *documenters[] = {
	"Dan Mueth <muet@alumni.uchicago.edu>",
	"John Fleck <jfleck@inkstain.net>",
	NULL
    };

    gtk_show_about_dialog (NULL,
	"version",     VERSION,
	"copyright",   copyright,
	"comments",    _("Applet for mounting and unmounting block volumes."),
	"authors",     authors,
	"documenters", documenters,
	"translator-credits", _("translator-credits"),
	"logo_icon_name",     "media-floppy",
	NULL);
}

static void
display_help (GtkAction *action,
	      DriveList *drive_list)
{
    GdkScreen *screen;
    GError *error = NULL;

    screen = gtk_widget_get_screen (GTK_WIDGET (drive_list));

    gtk_show_uri_on_window (NULL,
                           "help:mate-drivemount",
                            gtk_get_current_event_time (),
                            &error);

    if (error) {
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL,
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 _("There was an error displaying help: %s"),
					 error->message);
	g_signal_connect (dialog, "response",
			  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_screen (GTK_WINDOW (dialog), screen);
	gtk_widget_show (dialog);
	g_error_free (error);
    }
}

static const GtkActionEntry applet_menu_actions[] = {
    { "Help", GTK_STOCK_HELP, N_("_Help"),
      NULL, NULL,
      G_CALLBACK (display_help) },
    { "About", GTK_STOCK_ABOUT, N_("_About"),
      NULL, NULL,
      G_CALLBACK (display_about_dialog) }
};

static gboolean
applet_factory (MatePanelApplet *applet,
		const char  *iid,
		gpointer     user_data)
{
    gboolean ret = FALSE;
    GtkWidget *drive_list;
    AtkObject *ao;
    GtkActionGroup *action_group;
    gchar *ui_path;

    if (!strcmp (iid, drivemount_iid)) {
	g_set_application_name (_("Disk Mounter"));

	gtk_window_set_default_icon_name ("media-floppy");

	mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);
	mate_panel_applet_set_background_widget (applet, GTK_WIDGET (applet));

	settings = g_settings_new ( "org.mate.drivemount");

	drive_list = drive_list_new ();
    g_signal_connect(settings,
                     "changed::drivemount-checkmark-color",
                     G_CALLBACK (settings_color_changed),
                     drive_list);
	gtk_container_add (GTK_CONTAINER (applet), drive_list);

	g_signal_connect_object (applet, "change_orient",
				 G_CALLBACK (change_orient), drive_list, 0);
	g_signal_connect_object (applet, "size_allocate",
				 G_CALLBACK (size_allocate), drive_list, 0);
	g_signal_connect (applet, "change_background",
			  G_CALLBACK (change_background), drive_list);

	/* set initial state */
	change_orient (applet,
		       mate_panel_applet_get_orient (applet),
		       DRIVE_LIST (drive_list));

	action_group = gtk_action_group_new ("DriveMount Applet Actions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group,
				      applet_menu_actions,
				      G_N_ELEMENTS (applet_menu_actions),
				      drive_list);
	ui_path = g_build_filename (DRIVEMOUNT_MENU_UI_DIR, "drivemount-applet-menu.xml", NULL);
	mate_panel_applet_setup_menu_from_file (applet, ui_path, action_group);
	g_free (ui_path);
	g_object_unref (action_group);

	ao = gtk_widget_get_accessible (GTK_WIDGET (applet));
	atk_object_set_name (ao, _("Disk Mounter"));

	gtk_widget_show_all (GTK_WIDGET (applet));

	ret = TRUE;
    }

    return ret;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY (factory_iid,
				  PANEL_TYPE_APPLET,
				  "Drive-Mount-Applet",
				  applet_factory, NULL)
