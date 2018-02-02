/* -*- Mode: C; c-basic-offset: 4 -*-
 * geyes.c - A cheap xeyes ripoff.
 * Copyright (C) 1999 Dave Camp
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include <math.h>
#include <stdlib.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>
#include "geyes.h"

#define UPDATE_TIMEOUT 100

static gfloat
gtk_align_to_gfloat (GtkAlign align)
{
	switch (align) {
		case GTK_ALIGN_START:
			return 0.0;
		case GTK_ALIGN_END:
			return 1.0;
		case GTK_ALIGN_CENTER:
		case GTK_ALIGN_FILL:
			return 0.5;
		default:
			return 0.0;
	}
}

/* TODO - Optimize this a bit */
static void 
calculate_pupil_xy (EyesApplet *eyes_applet,
		    gint x, gint y,
		    gint *pupil_x, gint *pupil_y, GtkWidget* widget)
{
        GtkAllocation allocation;
        double sina;
        double cosa;
        double h;
        double temp;
 	 double nx, ny;

	 gfloat xalign, yalign;
	 gint width, height;

	 gtk_widget_get_allocation (GTK_WIDGET(widget), &allocation);
	 width = allocation.width;
	 height = allocation.height;
	 xalign = gtk_align_to_gfloat (gtk_widget_get_halign (widget));
	 yalign = gtk_align_to_gfloat (gtk_widget_get_valign (widget));

	 nx = x - MAX(width - eyes_applet->eye_width, 0) * xalign - eyes_applet->eye_width / 2;
	 ny = y - MAX(height- eyes_applet->eye_height, 0) * yalign - eyes_applet->eye_height / 2;
  
	 h = hypot (nx, ny);
        if (h < 0.5 || fabs (h)
            < (fabs (hypot (eyes_applet->eye_height / 2, eyes_applet->eye_width / 2)) - eyes_applet->wall_thickness - eyes_applet->pupil_height)) {
                *pupil_x = nx + eyes_applet->eye_width / 2;
                *pupil_y = ny + eyes_applet->eye_height / 2;
                return;
        }
        
	 sina = nx / h; 
	 cosa = ny / h;
	
        temp = hypot ((eyes_applet->eye_width / 2) * sina, (eyes_applet->eye_height / 2) * cosa);
        temp -= hypot ((eyes_applet->pupil_width / 2) * sina, (eyes_applet->pupil_height / 2) * cosa);
        temp -= hypot ((eyes_applet->wall_thickness / 2) * sina, (eyes_applet->wall_thickness / 2) * cosa);

        *pupil_x = temp * sina + (eyes_applet->eye_width / 2);
        *pupil_y = temp * cosa + (eyes_applet->eye_height / 2);
}

static void 
draw_eye (EyesApplet *eyes_applet,
	  gint eye_num, 
          gint pupil_x, 
          gint pupil_y)
{
	GdkPixbuf *pixbuf;
	GdkRectangle rect, r1, r2;

	pixbuf = gdk_pixbuf_copy (eyes_applet->eye_image);
	r1.x = pupil_x - eyes_applet->pupil_width / 2;
	r1.y = pupil_y - eyes_applet->pupil_height / 2;
	r1.width = eyes_applet->pupil_width;
	r1.height = eyes_applet->pupil_height;
	r2.x = 0;
	r2.y = 0;
	r2.width = eyes_applet->eye_width;
	r2.height = eyes_applet->eye_height;
	gdk_rectangle_intersect (&r1, &r2, &rect);
	gdk_pixbuf_composite (eyes_applet->pupil_image, pixbuf, 
					   rect.x,
					   rect.y,
					   rect.width,
				      	   rect.height,
				      	   pupil_x - eyes_applet->pupil_width / 2,
					   pupil_y - eyes_applet->pupil_height / 2, 1.0, 1.0,
				      	   GDK_INTERP_BILINEAR,
				           255);
	gtk_image_set_from_pixbuf (GTK_IMAGE (eyes_applet->eyes[eye_num]),
						  pixbuf);
	g_object_unref (pixbuf);

}

static gint 
timer_cb (EyesApplet *eyes_applet)
{
        GdkDisplay *display;
        GdkSeat *seat;
        gint x, y;
        gint pupil_x, pupil_y;
        gint i;

        display = gtk_widget_get_display (GTK_WIDGET (eyes_applet->applet));
        seat = gdk_display_get_default_seat (display);

        for (i = 0; i < eyes_applet->num_eyes; i++) {
		if (gtk_widget_get_realized (eyes_applet->eyes[i])) {
            gdk_window_get_device_position (gtk_widget_get_window (eyes_applet->eyes[i]),
                                            gdk_seat_get_pointer (seat),
                                            &x, &y, NULL);

			if ((x != eyes_applet->pointer_last_x[i]) || (y != eyes_applet->pointer_last_y[i])) { 

				calculate_pupil_xy (eyes_applet, x, y, &pupil_x, &pupil_y, eyes_applet->eyes[i]);
				draw_eye (eyes_applet, i, pupil_x, pupil_y);
	    	        
			        eyes_applet->pointer_last_x[i] = x;
			        eyes_applet->pointer_last_y[i] = y;
			}
		}
        }
        return TRUE;
}

static void
about_cb (GtkAction   *action,
	  EyesApplet  *eyes_applet)
{
    static const gchar *authors [] = {
                "Dave Camp <campd@oit.edu>",
                NULL
	};

    char copyright[] = \
                "Copyright \xc2\xa9 2012-2018 MATE developers\n"
                "Copyright \xC2\xA9 1999 Dave Camp";

	const gchar *documenters[] = {
                "Arjan Scherpenisse <acscherp@wins.uva.nl>",
                "Telsa Gwynne <hobbit@aloss.ukuu.org.uk>",
                "Sun GNOME Documentation Team <gdocteam@sun.com>",
		NULL
	};

	gtk_show_about_dialog (NULL,
		"version",            VERSION,
		"comments",           _("A goofy set of eyes for the MATE "
		                      "panel. They follow your mouse."),
		"copyright",          copyright,
		"authors",            authors,
		"documenters",        documenters,
		"translator-credits", _("translator-credits"),
		"logo-icon-name",     "mate-eyes-applet",
		NULL);
}

static int
properties_load (EyesApplet *eyes_applet)
{
        gchar *theme_path = NULL;

	theme_path = g_settings_get_string (eyes_applet->settings, "theme-path");

	if (theme_path == NULL)
		theme_path = g_strdup (GEYES_THEMES_DIR "Default-tiny");
	
        if (load_theme (eyes_applet, theme_path) == FALSE) {
		g_free (theme_path);

		return FALSE;
	}

        g_free (theme_path);
	
	return TRUE;
}

void
setup_eyes (EyesApplet *eyes_applet) 
{
	int i;

        eyes_applet->hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start (GTK_BOX (eyes_applet->vbox), eyes_applet->hbox, TRUE, TRUE, 0);

	eyes_applet->eyes = g_new0 (GtkWidget *, eyes_applet->num_eyes);
	eyes_applet->pointer_last_x = g_new0 (gint, eyes_applet->num_eyes);
	eyes_applet->pointer_last_y = g_new0 (gint, eyes_applet->num_eyes);

        for (i = 0; i < eyes_applet->num_eyes; i++) {
                eyes_applet->eyes[i] = gtk_image_new ();
                if (eyes_applet->eyes[i] == NULL)
                        g_error ("Error creating geyes\n");
               
		gtk_widget_set_size_request (GTK_WIDGET (eyes_applet->eyes[i]),
					     eyes_applet->eye_width,
					     eyes_applet->eye_height);
 
                gtk_widget_show (eyes_applet->eyes[i]);
                
		gtk_box_pack_start (GTK_BOX (eyes_applet->hbox), 
                                    eyes_applet->eyes [i],
                                    TRUE,
                                    TRUE,
                                    0);
                
		if ((eyes_applet->num_eyes != 1) && (i == 0)) {
			gtk_widget_set_halign (eyes_applet->eyes[i], GTK_ALIGN_END);
			gtk_widget_set_valign (eyes_applet->eyes[i], GTK_ALIGN_CENTER);
		}
		else if ((eyes_applet->num_eyes != 1) && (i == eyes_applet->num_eyes - 1)) {
			gtk_widget_set_halign (eyes_applet->eyes[i], GTK_ALIGN_START);
			gtk_widget_set_valign (eyes_applet->eyes[i], GTK_ALIGN_CENTER);
		}
		else {
			gtk_widget_set_halign (eyes_applet->eyes[i], GTK_ALIGN_CENTER);
			gtk_widget_set_valign (eyes_applet->eyes[i], GTK_ALIGN_CENTER);
		}
		
                gtk_widget_realize (eyes_applet->eyes[i]);
		
		eyes_applet->pointer_last_x[i] = G_MAXINT;
		eyes_applet->pointer_last_y[i] = G_MAXINT;
		
		draw_eye (eyes_applet, i,
			  eyes_applet->eye_width / 2,
                          eyes_applet->eye_height / 2);
                
        }
        gtk_widget_show (eyes_applet->hbox);
}

void
destroy_eyes (EyesApplet *eyes_applet)
{
	gtk_widget_destroy (eyes_applet->hbox);
	eyes_applet->hbox = NULL;

	g_free (eyes_applet->eyes);
	g_free (eyes_applet->pointer_last_x);
	g_free (eyes_applet->pointer_last_y);
}

static EyesApplet *
create_eyes (MatePanelApplet *applet)
{
	EyesApplet *eyes_applet = g_new0 (EyesApplet, 1);

        eyes_applet->applet = applet;
        eyes_applet->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	eyes_applet->settings = 
		mate_panel_applet_settings_new (applet, "org.mate.panel.applet.geyes");

	gtk_container_add (GTK_CONTAINER (applet), eyes_applet->vbox);

	return eyes_applet;
}

static void
dispose_cb (GObject *object, EyesApplet *eyes_applet)
{
	g_return_if_fail (eyes_applet);

	g_source_remove (eyes_applet->timeout_id);
	if (eyes_applet->hbox)
		destroy_eyes (eyes_applet);
	eyes_applet->timeout_id = 0;
	if (eyes_applet->eye_image)
		g_object_unref (eyes_applet->eye_image);
	eyes_applet->eye_image = NULL;
	if (eyes_applet->pupil_image)
		g_object_unref (eyes_applet->pupil_image);
	eyes_applet->pupil_image = NULL;
	if (eyes_applet->theme_dir)
		g_free (eyes_applet->theme_dir);
	eyes_applet->theme_dir = NULL;
	if (eyes_applet->theme_name)
		g_free (eyes_applet->theme_name);
	eyes_applet->theme_name = NULL;
	if (eyes_applet->eye_filename)
		g_free (eyes_applet->eye_filename);
	eyes_applet->eye_filename = NULL;
	if (eyes_applet->pupil_filename)
		g_free (eyes_applet->pupil_filename);
	eyes_applet->pupil_filename = NULL;
	
	if (eyes_applet->prop_box.pbox)
		gtk_widget_destroy (eyes_applet->prop_box.pbox);

	if (eyes_applet->settings)
		g_object_unref (eyes_applet->settings);
	eyes_applet->settings = NULL;

	g_free (eyes_applet);
}

static void
help_cb (GtkAction  *action,
	 EyesApplet *eyes_applet)
{
	GError *error = NULL;

	gtk_show_uri_on_window (NULL,
	                        "help:mate-geyes",
	                        gtk_get_current_event_time (),
	                        &error);

	if (error) {
		GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
							    _("There was an error displaying help: %s"), error->message);
		g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (GTK_WIDGET (eyes_applet->applet)));
		gtk_widget_show (dialog);
		g_error_free (error);
		error = NULL;
	}
}


static const GtkActionEntry geyes_applet_menu_actions [] = {
	{ "Props", GTK_STOCK_PROPERTIES, N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (properties_cb) },
	{ "Help", GTK_STOCK_HELP, N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (help_cb) },
	{ "About", GTK_STOCK_ABOUT, N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (about_cb) }
};

static void
set_atk_name_description (GtkWidget *widget, const gchar *name,
    const gchar *description)
{
	AtkObject *aobj;
   
	aobj = gtk_widget_get_accessible (widget);
	/* Check if gail is loaded */
	if (GTK_IS_ACCESSIBLE (aobj) == FALSE)
		return;

	atk_object_set_name (aobj, name);
	atk_object_set_description (aobj, description);
}

static gboolean
geyes_applet_fill (MatePanelApplet *applet)
{
	EyesApplet *eyes_applet;
	GtkActionGroup *action_group;
	gchar *ui_path;

	g_set_application_name (_("Eyes"));
	
	gtk_window_set_default_icon_name ("mate-eyes-applet");
	mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);
	mate_panel_applet_set_background_widget (applet, GTK_WIDGET (applet));
	
        eyes_applet = create_eyes (applet);

        eyes_applet->timeout_id = g_timeout_add (
		UPDATE_TIMEOUT, (GSourceFunc) timer_cb, eyes_applet);

	action_group = gtk_action_group_new ("Geyes Applet Actions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group,
				      geyes_applet_menu_actions,
				      G_N_ELEMENTS (geyes_applet_menu_actions),
				      eyes_applet);
	ui_path = g_build_filename (GEYES_MENU_UI_DIR, "geyes-applet-menu.xml", NULL);
	mate_panel_applet_setup_menu_from_file (eyes_applet->applet,
					   ui_path, action_group);
	g_free (ui_path);

	if (mate_panel_applet_get_locked_down (eyes_applet->applet)) {
		GtkAction *action;

		action = gtk_action_group_get_action (action_group, "Props");
		gtk_action_set_visible (action, FALSE);
	}
	g_object_unref (action_group);

	gtk_widget_set_tooltip_text (GTK_WIDGET (eyes_applet->applet), _("Eyes"));

	set_atk_name_description (GTK_WIDGET (eyes_applet->applet), _("Eyes"), 
			_("The eyes look in the direction of the mouse pointer"));

	g_signal_connect (eyes_applet->vbox,
			  "dispose",
			  G_CALLBACK (dispose_cb),
			  eyes_applet);

	gtk_widget_show_all (GTK_WIDGET (eyes_applet->applet));

	/* setup here and not in create eyes so the destroy signal is set so 
	 * that when there is an error within loading the theme
	 * we can emit this signal */
        if (properties_load (eyes_applet) == FALSE)
		return FALSE;

	setup_eyes (eyes_applet);

	return TRUE;
}

static gboolean
geyes_applet_factory (MatePanelApplet *applet,
		      const gchar *iid,
		      gpointer     data)
{
	gboolean retval = FALSE;

	theme_dirs_create ();

	if (!strcmp (iid, "GeyesApplet"))
		retval = geyes_applet_fill (applet); 
   
	if (retval == FALSE) {
		exit (-1);
	}

	return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("GeyesAppletFactory",
				  PANEL_TYPE_APPLET,
				  "geyes",
				  geyes_applet_factory,
				  NULL)
