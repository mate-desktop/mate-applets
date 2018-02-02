/* Sticky Notes
 * Copyright (C) 2002-2003 Loban A Rahman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>
#include <libxml/parser.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>
#include <string.h>
#include <sys/stat.h>

#include <gtksourceview/gtksource.h>

#include "stickynotes.h"
#include "stickynotes_callbacks.h"
#include "util.h"
#include "stickynotes_applet.h"

/* Stop gcc complaining about xmlChar's signedness */
#define XML_CHAR(str) ((xmlChar *) (str))
#define STICKYNOTES_ICON_SIZE 8

static gboolean save_scheduled = FALSE;

static void response_cb (GtkWidget *dialog, gint id, gpointer data);

/* Based on a function found in wnck */
static void
set_icon_geometry (GdkWindow *window,
                   int        x,
                   int        y,
                   int        width,
                   int        height)
{
	gulong data[4];
	Display *dpy;

	dpy = gdk_x11_display_get_xdisplay (gdk_window_get_display (window));

	data[0] = x;
	data[1] = y;
	data[2] = width;
	data[3] = height;

	XChangeProperty (dpy,
	                 GDK_WINDOW_XID (window),
	                 gdk_x11_get_xatom_by_name_for_display (
	                     gdk_window_get_display (window),
	                     "_NET_WM_ICON_GEOMETRY"),
	                 XA_CARDINAL, 32, PropModeReplace,
	                 (guchar *)&data, 4);
}

/* Called when a timeout occurs.  */
static gboolean
timeout_happened (gpointer data)
{
	if (GPOINTER_TO_UINT (data) == stickynotes->last_timeout_data)
		stickynotes_save ();
	return FALSE;
}

/* Called when a text buffer is changed.  */
static void
buffer_changed (GtkTextBuffer *buffer, StickyNote *note)
{
	if ( (note->h + note->y) > stickynotes->max_height )
		gtk_scrolled_window_set_policy ( GTK_SCROLLED_WINDOW(note->w_scroller),
													GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/* When a buffer is changed, we set a 10 second timer.  When
	   the timer triggers, we will save the buffer if there have
	   been no subsequent changes.  */
	++stickynotes->last_timeout_data;
	g_timeout_add_seconds (10, (GSourceFunc) timeout_happened,
		       GUINT_TO_POINTER (stickynotes->last_timeout_data));
}

/* Create a new (empty) Sticky Note at a specific position
   and with specific size */
static StickyNote *
stickynote_new_aux (GdkScreen *screen, gint x, gint y, gint w, gint h)
{
	StickyNote *note;
	GtkBuilder *builder;

	note = g_new (StickyNote, 1);

	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, BUILDER_PATH, NULL);

	note->w_window = GTK_WIDGET (gtk_builder_get_object (builder, "stickynote_window"));
	gtk_window_set_screen(GTK_WINDOW(note->w_window),screen);
	gtk_window_set_decorated (GTK_WINDOW (note->w_window), FALSE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (note->w_window), TRUE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (note->w_window), TRUE);
	gtk_widget_add_events (note->w_window, GDK_BUTTON_PRESS_MASK);

	note->w_title = GTK_WIDGET (gtk_builder_get_object (builder, "title_label"));
	note->w_body = GTK_WIDGET (gtk_builder_get_object (builder, "body_text"));
	note->w_scroller = GTK_WIDGET (gtk_builder_get_object (builder, "body_scroller"));
	note->w_lock = GTK_WIDGET (gtk_builder_get_object (builder, "lock_button"));
	gtk_widget_add_events (note->w_lock, GDK_BUTTON_PRESS_MASK);

	note->buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(note->w_body)));

	note->w_close = GTK_WIDGET (gtk_builder_get_object (builder, "close_button"));
	gtk_widget_add_events (note->w_close, GDK_BUTTON_PRESS_MASK);
	note->w_resize_se = GTK_WIDGET (gtk_builder_get_object (builder, "resize_se_box"));
	gtk_widget_add_events (note->w_resize_se, GDK_BUTTON_PRESS_MASK);
	note->w_resize_sw = GTK_WIDGET (gtk_builder_get_object (builder, "resize_sw_box"));
	gtk_widget_add_events (note->w_resize_sw, GDK_BUTTON_PRESS_MASK);

	note->img_lock = GTK_IMAGE (gtk_builder_get_object (builder,
	                "lock_img"));
	note->img_close = GTK_IMAGE (gtk_builder_get_object (builder,
	                "close_img"));
	note->img_resize_se = GTK_IMAGE (gtk_builder_get_object (builder,
	                "resize_se_img"));
	note->img_resize_sw = GTK_IMAGE (gtk_builder_get_object (builder,
	                "resize_sw_img"));

	/* deal with RTL environments */
	gtk_widget_set_direction (GTK_WIDGET (gtk_builder_get_object (builder, "resize_bar")),
			GTK_TEXT_DIR_LTR);

	note->w_menu = GTK_WIDGET (gtk_builder_get_object (builder, "stickynote_menu"));
	note->ta_lock_toggle_item = GTK_TOGGLE_ACTION (gtk_builder_get_object (builder,
	        "popup_toggle_lock"));

	note->w_properties = GTK_WIDGET (gtk_builder_get_object (builder,
			"stickynote_properties"));
	gtk_window_set_screen (GTK_WINDOW (note->w_properties), screen);

	note->w_entry = GTK_WIDGET (gtk_builder_get_object (builder, "title_entry"));
	note->w_color = GTK_WIDGET (gtk_builder_get_object (builder, "note_color"));
	note->w_color_label = GTK_WIDGET (gtk_builder_get_object (builder, "color_label"));
	note->w_font_color = GTK_WIDGET (gtk_builder_get_object (builder, "font_color"));
	note->w_font_color_label = GTK_WIDGET (gtk_builder_get_object (builder,
			"font_color_label"));
	note->w_font = GTK_WIDGET (gtk_builder_get_object (builder, "note_font"));
	note->w_font_label = GTK_WIDGET (gtk_builder_get_object (builder, "font_label"));
	note->w_def_color = GTK_WIDGET (&GTK_CHECK_BUTTON (
				gtk_builder_get_object (builder,
					"def_color_check"))->toggle_button);
	note->w_def_font = GTK_WIDGET (&GTK_CHECK_BUTTON (
				gtk_builder_get_object (builder,
					"def_font_check"))->toggle_button);

	note->color = NULL;
	note->font_color = NULL;
	note->font = NULL;
	note->locked = FALSE;
	note->x = x;
	note->y = y;
	note->w = w;
	note->h = h;

	/* Customize the window */
	if (g_settings_get_boolean (stickynotes->settings, "sticky"))
		gtk_window_stick(GTK_WINDOW(note->w_window));

	if (w == 0 || h == 0)
		gtk_window_resize (GTK_WINDOW(note->w_window),
				g_settings_get_int (stickynotes->settings, "default-width"),
				g_settings_get_int (stickynotes->settings, "default-height"));
	else
		gtk_window_resize (GTK_WINDOW(note->w_window),
				note->w,
				note->h);

	if (x != -1 && y != -1)
		gtk_window_move (GTK_WINDOW(note->w_window),
				note->x,
				note->y);

	/* Set the button images */
	gtk_image_set_from_icon_name (note->img_close, STICKYNOTES_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_image_set_pixel_size (note->img_close, STICKYNOTES_ICON_SIZE);

	gtk_image_set_from_icon_name (note->img_resize_se, STICKYNOTES_STOCK_RESIZE_SE, GTK_ICON_SIZE_MENU);
	gtk_image_set_pixel_size (note->img_resize_se, STICKYNOTES_ICON_SIZE);

	gtk_image_set_from_icon_name (note->img_resize_sw, STICKYNOTES_STOCK_RESIZE_SW, GTK_ICON_SIZE_MENU);
	gtk_image_set_pixel_size (note->img_resize_sw, STICKYNOTES_ICON_SIZE);

	gtk_widget_show(note->w_lock);
	gtk_widget_show(note->w_close);
	gtk_widget_show(GTK_WIDGET (gtk_builder_get_object (builder, "resize_bar")));

	/* Customize the title and colors, hide and unlock */
	stickynote_set_title(note, NULL);
	stickynote_set_color(note, NULL, NULL, TRUE);
	stickynote_set_font(note, NULL, TRUE);
	stickynote_set_locked(note, FALSE);

	gtk_widget_realize (note->w_window);

	/* Connect a popup menu to all buttons and title */
	/* GtkBuilder holds and drops the references to all the widgets it
	 * creates for as long as it exist (GtkBuilder). Hence in our callback
	 * we would have an invalid GtkMenu. We need to ref it.
	 */
	g_object_ref (note->w_menu);
	g_signal_connect (G_OBJECT (note->w_window), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	g_signal_connect (G_OBJECT (note->w_lock), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	g_signal_connect (G_OBJECT (note->w_close), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	g_signal_connect (G_OBJECT (note->w_resize_se), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	g_signal_connect (G_OBJECT (note->w_resize_sw), "button-press-event",
			G_CALLBACK (stickynote_show_popup_menu), note->w_menu);

	/* Connect a properties dialog to the note */
	gtk_window_set_transient_for (GTK_WINDOW(note->w_properties),
			GTK_WINDOW(note->w_window));
	gtk_dialog_set_default_response (GTK_DIALOG(note->w_properties),
			GTK_RESPONSE_CLOSE);
	g_signal_connect (G_OBJECT (note->w_properties), "response",
			G_CALLBACK (response_cb), note);

	/* Connect signals to the sticky note */
	g_signal_connect (G_OBJECT (note->w_lock), "clicked",
			G_CALLBACK (stickynote_toggle_lock_cb), note);
	g_signal_connect (G_OBJECT (note->w_close), "clicked",
			G_CALLBACK (stickynote_close_cb), note);
	g_signal_connect (G_OBJECT (note->w_resize_se), "button-press-event",
			G_CALLBACK (stickynote_resize_cb), note);
	g_signal_connect (G_OBJECT (note->w_resize_sw), "button-press-event",
			G_CALLBACK (stickynote_resize_cb), note);

	g_signal_connect (G_OBJECT (note->w_window), "button-press-event",
			G_CALLBACK (stickynote_move_cb), note);
	g_signal_connect (G_OBJECT (note->w_window), "configure-event",
			G_CALLBACK (stickynote_configure_cb), note);
	g_signal_connect (G_OBJECT (note->w_window), "delete-event",
			G_CALLBACK (stickynote_delete_cb), note);

	g_signal_connect (gtk_builder_get_object (builder,
					"popup_create"), "activate",
			G_CALLBACK (popup_create_cb), note);
	g_signal_connect (gtk_builder_get_object (builder,
					"popup_destroy"), "activate",
			G_CALLBACK (popup_destroy_cb), note);
	g_signal_connect (gtk_builder_get_object (builder,
					"popup_toggle_lock"), "toggled",
			G_CALLBACK (popup_toggle_lock_cb), note);
	g_signal_connect (gtk_builder_get_object (builder,
					"popup_properties"), "activate",
			G_CALLBACK (popup_properties_cb), note);

	g_signal_connect_swapped (G_OBJECT (note->w_entry), "changed",
			G_CALLBACK (properties_apply_title_cb), note);
	g_signal_connect (G_OBJECT (note->w_color), "color-set",
			G_CALLBACK (properties_color_cb), note);
	g_signal_connect (G_OBJECT (note->w_font_color), "color-set",
			G_CALLBACK (properties_color_cb), note);
	g_signal_connect_swapped (G_OBJECT (note->w_def_color), "toggled",
			G_CALLBACK (properties_apply_color_cb), note);
	g_signal_connect (G_OBJECT (note->w_font), "font-set",
			G_CALLBACK (properties_font_cb), note);
	g_signal_connect_swapped (G_OBJECT (note->w_def_font), "toggled",
			G_CALLBACK (properties_apply_font_cb), note);
	g_signal_connect (G_OBJECT (note->w_entry), "activate",
			G_CALLBACK (properties_activate_cb), note);
	g_signal_connect (G_OBJECT (note->w_properties), "delete-event",
			G_CALLBACK (gtk_widget_hide), note);

	g_object_unref(builder);

	g_signal_connect_after (note->w_body, "button-press-event",
	                        G_CALLBACK (gtk_true), note);

	g_signal_connect (gtk_text_view_get_buffer(GTK_TEXT_VIEW(note->w_body)),
			  "changed",
			  G_CALLBACK (buffer_changed), note);

	return note;
}

/* Create a new (empty) Sticky Note */
StickyNote *
stickynote_new (GdkScreen *screen)
{
	return stickynote_new_aux (screen, -1, -1, 0, 0);
}

/* Destroy a Sticky Note */
void stickynote_free(StickyNote *note)
{
	gtk_widget_destroy(note->w_properties);
	gtk_widget_destroy(note->w_menu);
	gtk_widget_destroy(note->w_window);

	g_free(note->color);
	g_free(note->font);

	g_free(note);
}

/* Change the sticky note title and color */
void stickynote_change_properties (StickyNote *note)
{
	char *color_str = NULL;

	gtk_entry_set_text(GTK_ENTRY(note->w_entry),
			gtk_label_get_text (GTK_LABEL (note->w_title)));

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(note->w_def_color),
			note->color == NULL);

	if (note->color)
		color_str = g_strdup (note->color);
	else
	{
		color_str = g_settings_get_string (stickynotes->settings, "default-color");
	}

	if (color_str)
	{
		GdkRGBA color;
		gdk_rgba_parse (&color, color_str);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (note->w_color), &color);
		g_free (color_str);
	}

	if (note->font_color)
		color_str = g_strdup (note->font_color);
	else
	{
		color_str = g_settings_get_string (stickynotes->settings, "default-font-color");
	}

	if (color_str)
	{
		GdkRGBA font_color;
		gdk_rgba_parse (&font_color, color_str);
		gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (note->w_font_color), &font_color);
		g_free (color_str);
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(note->w_def_font),
			note->font == NULL);
	if (note->font)
		gtk_font_button_set_font_name (GTK_FONT_BUTTON (note->w_font),
				note->font);

	gtk_widget_show (note->w_properties);

	stickynotes_save();
}

static void
response_cb (GtkWidget *dialog, gint id, gpointer data)
{
        if (id == GTK_RESPONSE_HELP)
		gtk_show_uri_on_window (GTK_WINDOW (dialog),
		                        "help:mate-stickynotes-applet/stickynotes-settings-individual",
		                        gtk_get_current_event_time (),
		                        NULL);
        else if (id == GTK_RESPONSE_CLOSE)
                gtk_widget_hide (dialog);
}

/* Check if a sticky note is empty */
gboolean stickynote_get_empty(const StickyNote *note)
{
	return gtk_text_buffer_get_char_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW(note->w_body))) == 0;
}

/* Set the sticky note title */
void stickynote_set_title(StickyNote *note, const gchar *title)
{
	/* If title is NULL, use the current date as the title. */
	if (!title) {
		gchar *date_title, *tmp;
		gchar *date_format = g_settings_get_string (stickynotes->settings, "date-format");
		if (!date_format)
			date_format = g_strdup ("%x");
		tmp = get_current_date (date_format);
		date_title = g_locale_to_utf8 (tmp, -1, NULL, NULL, NULL);

		gtk_window_set_title(GTK_WINDOW(note->w_window), date_title);
		gtk_label_set_text(GTK_LABEL (note->w_title), date_title);

		g_free (tmp);
		g_free(date_title);
		g_free(date_format);
	}
	else {
		gtk_window_set_title(GTK_WINDOW(note->w_window), title);
		gtk_label_set_text(GTK_LABEL (note->w_title), title);
	}
}

/* Set the sticky note color */
void
stickynote_set_color (StickyNote  *note,
		      const gchar *color_str,
		      const gchar *font_color_str,
		      gboolean     save)
{
	char *color_str_actual, *font_color_str_actual;
	gboolean force_default, use_system_color;

	if (save) {
		if (note->color)
			g_free (note->color);
		if (note->font_color)
			g_free (note->font_color);

		note->color = color_str ?
			g_strdup (color_str) : NULL;
		note->font_color = font_color_str ?
			g_strdup (font_color_str) : NULL;

		gtk_widget_set_sensitive (note->w_color_label,
				note->color != NULL);
		gtk_widget_set_sensitive (note->w_font_color_label,
				note->font_color != NULL);
		gtk_widget_set_sensitive (note->w_color,
				note->color != NULL);
		gtk_widget_set_sensitive (note->w_font_color,
				note->color != NULL);
	}

	force_default = g_settings_get_boolean (stickynotes->settings, "force-default");
	use_system_color = g_settings_get_boolean (stickynotes->settings, "use-system-color");

	/* If "force_default" is enabled or color_str is NULL,
	 * then we use the default color instead of color_str. */
	if (!color_str || force_default)
	{
		if (use_system_color)
			color_str_actual = NULL;
		else
			color_str_actual = g_settings_get_string (stickynotes->settings, "default-color");
	}
	else
		color_str_actual = g_strdup (color_str);

	if (!font_color_str || force_default)
	{
		if (use_system_color)
			font_color_str_actual = NULL;
		else
			font_color_str_actual = g_settings_get_string (stickynotes->settings, "default-font-color");
	}
	else
		font_color_str_actual = g_strdup (font_color_str);

	/* Do not use custom colors if "use_system_color" is enabled */
	if (color_str_actual) {
		GdkRGBA colors[4];
		gint i;

		for (i = 0; i <= 3; i++)
		{
			gdk_rgba_parse (&colors[i], color_str_actual);
			colors[i].red = (colors[i].red * (10 - i)) / 10;
			colors[i].green = (colors[i].green * (10 - i)) / 10;
			colors[i].blue = (colors[i].blue * (10 - i)) / 10;
		}

		gtk_widget_override_background_color (note->w_window, GTK_STATE_NORMAL, &colors[0]);
		gtk_widget_override_background_color (note->w_body, GTK_STATE_NORMAL, &colors[0]);
		gtk_widget_override_background_color (note->w_lock, GTK_STATE_NORMAL, &colors[0]);
		gtk_widget_override_background_color (note->w_close, GTK_STATE_NORMAL, &colors[0]);
		gtk_widget_override_background_color (note->w_resize_se, GTK_STATE_NORMAL, &colors[0]);
		gtk_widget_override_background_color (note->w_resize_sw, GTK_STATE_NORMAL, &colors[0]);
	} else {
		gtk_widget_override_background_color (note->w_window, GTK_STATE_NORMAL, NULL);
		gtk_widget_override_background_color (note->w_body, GTK_STATE_NORMAL, NULL);
		gtk_widget_override_background_color (note->w_lock, GTK_STATE_NORMAL, NULL);
		gtk_widget_override_background_color (note->w_close, GTK_STATE_NORMAL, NULL);
		gtk_widget_override_background_color (note->w_resize_se, GTK_STATE_NORMAL, NULL);
		gtk_widget_override_background_color (note->w_resize_sw, GTK_STATE_NORMAL, NULL);
	}

	if (font_color_str_actual)
	{
		GdkRGBA color;

		gdk_rgba_parse (&color, font_color_str_actual);

		gtk_widget_override_color (note->w_window, GTK_STATE_NORMAL, &color);
		gtk_widget_override_color (note->w_body, GTK_STATE_NORMAL, &color);
	}
	else
	{
		gtk_widget_override_color (note->w_window, GTK_STATE_NORMAL, NULL);
		gtk_widget_override_color (note->w_body, GTK_STATE_NORMAL, NULL);
	}

	if (color_str_actual)
		g_free (color_str_actual);
	if (font_color_str_actual)
		g_free (font_color_str_actual);
}

/* Set the sticky note font */
void
stickynote_set_font (StickyNote *note, const gchar *font_str, gboolean save)
{
	PangoFontDescription *font_desc;
	gchar *font_str_actual;

	if (save) {
		g_free (note->font);
		note->font = font_str ? g_strdup (font_str) : NULL;

		gtk_widget_set_sensitive (note->w_font_label, note->font != NULL);
		gtk_widget_set_sensitive(note->w_font, note->font != NULL);
	}

	/* If "force_default" is enabled or font_str is NULL,
	 * then we use the default font instead of font_str. */
	if (!font_str || g_settings_get_boolean (stickynotes->settings, "force-default"))
	{
		if (g_settings_get_boolean (stickynotes->settings, "use-system-font"))
			font_str_actual = NULL;
		else
			font_str_actual = g_settings_get_string (stickynotes->settings, "default-font");
	}
	else
		font_str_actual = g_strdup (font_str);

	/* Do not use custom fonts if "use_system_font" is enabled */
	font_desc = font_str_actual ?
		pango_font_description_from_string (font_str_actual): NULL;

	/* Apply the style to the widgets */
	gtk_widget_override_font (note->w_window, font_desc);
	gtk_widget_override_font (note->w_body, font_desc);

	g_free (font_str_actual);
	pango_font_description_free (font_desc);
}

/* Lock/Unlock a sticky note from editing */
void stickynote_set_locked(StickyNote *note, gboolean locked)
{
	note->locked = locked;

	/* Set cursor visibility and editability */
	gtk_text_view_set_editable(GTK_TEXT_VIEW(note->w_body), !locked);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(note->w_body), !locked);

	/* Show appropriate icon and tooltip */
	if (locked) {
		gtk_image_set_from_icon_name (note->img_lock, STICKYNOTES_STOCK_LOCKED, GTK_ICON_SIZE_MENU);
		gtk_widget_set_tooltip_text(note->w_lock, _("This note is locked."));
	}
	else {
		gtk_image_set_from_icon_name (note->img_lock, STICKYNOTES_STOCK_UNLOCKED, GTK_ICON_SIZE_MENU);
		gtk_widget_set_tooltip_text(note->w_lock, _("This note is unlocked."));
	}

	gtk_image_set_pixel_size (note->img_lock, STICKYNOTES_ICON_SIZE);

	gtk_toggle_action_set_active(note->ta_lock_toggle_item, locked);

	stickynotes_applet_update_menus();
}

/* Show/Hide a sticky note */
void
stickynote_set_visible (StickyNote *note, gboolean visible)
{
	if (visible)
	{
		gtk_window_present (GTK_WINDOW (note->w_window));

		if (note->x != -1 || note->y != -1)
			gtk_window_move (GTK_WINDOW (note->w_window),
					note->x, note->y);
		/* Put the note on all workspaces if necessary. */
		if (g_settings_get_boolean (stickynotes->settings, "sticky"))
			gtk_window_stick(GTK_WINDOW(note->w_window));
		else if (note->workspace > 0)
		{
#if 0
			WnckWorkspace *wnck_ws;
			gulong xid;
			WnckWindow *wnck_win;
			WnckScreen *wnck_screen;

			g_print ("set_visible(): workspace = %i\n",
					note->workspace);

			xid = GDK_WINDOW_XID (note->w_window->window);
			wnck_screen = wnck_screen_get_default ();
			wnck_win = wnck_window_get (xid);
			wnck_ws = wnck_screen_get_workspace (
					wnck_screen,
					note->workspace - 1);
			if (wnck_win && wnck_ws)
				wnck_window_move_to_workspace (
						wnck_win, wnck_ws);
			else
				g_print ("set_visible(): errr\n");
#endif
			xstuff_change_workspace (GTK_WINDOW (note->w_window),
					note->workspace - 1);
		}
	}
	else {
		/* Hide sticky note */
		int x, y, width, height;
		stickynotes_applet_panel_icon_get_geometry (&x, &y, &width, &height);
		set_icon_geometry (gtk_widget_get_window (GTK_WIDGET (note->w_window)),
				   x, y, width, height);
		gtk_window_iconify(GTK_WINDOW (note->w_window));
	}
}

/* Add a sticky note */
void stickynotes_add (GdkScreen *screen)
{
	StickyNote *note;

	note = stickynote_new (screen);

	stickynotes->notes = g_list_append(stickynotes->notes, note);
	stickynotes_applet_update_tooltips();
	stickynotes_save();
	stickynote_set_visible (note, TRUE);
}

/* Remove a sticky note with confirmation, if needed */
void stickynotes_remove(StickyNote *note)
{
	GtkBuilder *builder;
	GtkWidget *dialog;

	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, BUILDER_PATH, NULL);

	dialog = GTK_WIDGET (gtk_builder_get_object (builder, "delete_dialog"));

	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(note->w_window));

	if (stickynote_get_empty(note)
	    || !g_settings_get_boolean (stickynotes->settings, "confirm-deletion")
	    || gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		stickynote_free(note);

		/* Remove the note from the linked-list of all notes */
		stickynotes->notes = g_list_remove(stickynotes->notes, note);

		/* Update tooltips */
		stickynotes_applet_update_tooltips();

		/* Save notes */
		stickynotes_save();
	}

	gtk_widget_destroy(dialog);
	g_object_unref(builder);
}

/* Save all sticky notes in an XML configuration file */
gboolean
stickynotes_save_now (void)
{
	WnckScreen *wnck_screen;
	const gchar *title;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *body;

	gint i;

	/* Create a new XML document */
	xmlDocPtr doc = xmlNewDoc(XML_CHAR ("1.0"));
	xmlNodePtr root = xmlNewDocNode(doc, NULL, XML_CHAR ("stickynotes"), NULL);

	xmlDocSetRootElement(doc, root);
	xmlNewProp(root, XML_CHAR("version"), XML_CHAR (VERSION));

	wnck_screen = wnck_screen_get_default ();
	wnck_screen_force_update (wnck_screen);

	/* For all sticky notes */
	for (i = 0; i < g_list_length(stickynotes->notes); i++) {
		WnckWindow *wnck_win;
		gulong xid = 0;

		/* Access the current note in the list */
		StickyNote *note = g_list_nth_data(stickynotes->notes, i);

		/* Retrieve the window size of the note */
		gchar *w_str = g_strdup_printf("%d", note->w);
		gchar *h_str = g_strdup_printf("%d", note->h);

		/* Retrieve the window position of the note */
		gchar *x_str = g_strdup_printf("%d", note->x);
		gchar *y_str = g_strdup_printf("%d", note->y);

		xid = GDK_WINDOW_XID (gtk_widget_get_window (note->w_window));
		wnck_win = wnck_window_get (xid);

		if (!g_settings_get_boolean (stickynotes->settings, "sticky") &&
			wnck_win)
			note->workspace = 1 +
				wnck_workspace_get_number (
				wnck_window_get_workspace (wnck_win));
		else
			note->workspace = 0;

		/* Retrieve the title of the note */
		title = gtk_label_get_text(GTK_LABEL(note->w_title));

		/* Retrieve body contents of the note */
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(note->w_body));

		gtk_text_buffer_get_bounds(buffer, &start, &end);
		body = gtk_text_iter_get_text(&start, &end);

		/* Save the note as a node in the XML document */
		{
			xmlNodePtr node = xmlNewTextChild(root, NULL, XML_CHAR ("note"),
					XML_CHAR (body));
			xmlNewProp(node, XML_CHAR ("title"), XML_CHAR (title));
			if (note->color)
				xmlNewProp (node, XML_CHAR ("color"), XML_CHAR (note->color));
			if (note->font_color)
				xmlNewProp (node, XML_CHAR ("font_color"),
						XML_CHAR (note->font_color));
			if (note->font)
				xmlNewProp (node, XML_CHAR ("font"), XML_CHAR (note->font));
			if (note->locked)
				xmlNewProp (node, XML_CHAR ("locked"), XML_CHAR ("true"));
			xmlNewProp (node, XML_CHAR ("x"), XML_CHAR (x_str));
			xmlNewProp (node, XML_CHAR ("y"), XML_CHAR (y_str));
			xmlNewProp (node, XML_CHAR ("w"), XML_CHAR (w_str));
			xmlNewProp (node, XML_CHAR ("h"), XML_CHAR (h_str));
			if (note->workspace > 0)
			{
				char *workspace_str;

				workspace_str = g_strdup_printf ("%i",
						note->workspace);
				xmlNewProp (node, XML_CHAR ("workspace"), XML_CHAR (workspace_str));
				g_free (workspace_str);
			}
		}

		/* Now that it has been saved, reset the modified flag */
		gtk_text_buffer_set_modified(buffer, FALSE);

		g_free(x_str);
		g_free(y_str);
		g_free(w_str);
		g_free(h_str);
		g_free(body);
	}

	/* The XML file is $HOME/.config/mate/stickynotes-applet, most probably */
	{
		gchar* path = g_build_filename(g_get_user_config_dir(), "mate", NULL);
		gchar* file = g_build_filename(path, "stickynotes-applet.xml", NULL);
		g_mkdir_with_parents(path, S_IRWXU);
		g_free(path);

		xmlSaveFormatFile(file, doc, 1);

		g_free(file);
	}

	xmlFreeDoc(doc);

	save_scheduled = FALSE;

	return FALSE;
}

void
stickynotes_save (void)
{
	/* If a save isn't already scheduled, save everything a minute from now. */
	if (!save_scheduled) {
		g_timeout_add_seconds (60, (GSourceFunc) stickynotes_save_now, NULL);
		save_scheduled = TRUE;
	}
}

/* Load all sticky notes from an XML configuration file */
void
stickynotes_load (GdkScreen *screen)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr root;
	xmlNodePtr node;
	/* WnckScreen *wnck_screen; */
	GList *new_notes, *tmp1;  /* Lists of StickyNote*'s */
	GList *new_nodes; /* Lists of xmlNodePtr's */
	int x, y, w, h;

	/* The XML file is $HOME/.config/mate/stickynotes-applet, most probably */
	{
		gchar* file = g_build_filename(g_get_user_config_dir(), "mate", "stickynotes-applet.xml", NULL);

		if (g_file_test(file, G_FILE_TEST_EXISTS))
		{
			/* load file */
		doc = xmlParseFile(file);
		}
		else
		{
			/* old one */
			g_free(file);

			file = g_build_filename(g_get_home_dir(), ".mate2", "stickynotes_applet", NULL);

			if (g_file_test(file, G_FILE_TEST_EXISTS))
			{
				/* load file */
				doc = xmlParseFile(file);
			}
		}

		g_free(file);
	}

	/* If the XML file does not exist, create a blank one */
	if (!doc)
	{
		stickynotes_save();
		return;
	}

	/* If the XML file is corrupted/incorrect, create a blank one */
	root = xmlDocGetRootElement(doc);
	if (!root || xmlStrcmp(root->name, XML_CHAR ("stickynotes")))
	{
		xmlFreeDoc(doc);
		stickynotes_save();
		return;
	}

	node = root->xmlChildrenNode;

	/* For all children of the root node (ie all sticky notes) */
	new_notes = NULL;
	new_nodes = NULL;
	while (node) {
		if (!xmlStrcmp(node->name, (const xmlChar *) "note"))
		{
			StickyNote *note;

			/* Retrieve and set the window size of the note */
			{
				gchar *w_str = (gchar *)xmlGetProp (node, XML_CHAR ("w"));
				gchar *h_str = (gchar *)xmlGetProp (node, XML_CHAR ("h"));
				if (w_str && h_str)
				{
					w = atoi (w_str);
					h = atoi (h_str);
				}
				else
				{
					w = 0;
					h = 0;
				}

				g_free (w_str);
				g_free (h_str);
			}

			/* Retrieve and set the window position of the note */
			{
				gchar *x_str = (gchar *)xmlGetProp (node, XML_CHAR ("x"));
				gchar *y_str = (gchar *)xmlGetProp (node, XML_CHAR ("y"));

				if (x_str && y_str)
				{
					x = atoi (x_str);
					y = atoi (y_str);
				}
				else
				{
					x = -1;
					y = -1;
				}

				g_free (x_str);
				g_free (y_str);
			}

			/* Create a new note */
			note = stickynote_new_aux (screen, x, y, w, h);
			stickynotes->notes = g_list_append (stickynotes->notes,
					note);
			new_notes = g_list_append (new_notes, note);
			new_nodes = g_list_append (new_nodes, node);

			/* Retrieve and set title of the note */
			{
				gchar *title = (gchar *)xmlGetProp(node, XML_CHAR ("title"));
				if (title)
					stickynote_set_title (note, title);
				g_free (title);
			}

			/* Retrieve and set the color of the note */
			{
				gchar *color_str;
				gchar *font_color_str;

				color_str = (gchar *)xmlGetProp (node, XML_CHAR ("color"));
				font_color_str = (gchar *)xmlGetProp (node, XML_CHAR ("font_color"));

				if (color_str || font_color_str)
					stickynote_set_color (note,
							color_str,
							font_color_str,
							TRUE);
				g_free (color_str);
				g_free (font_color_str);
			}

			/* Retrieve and set the font of the note */
			{
				gchar *font_str = (gchar *)xmlGetProp (node, XML_CHAR ("font"));
				if (font_str)
					stickynote_set_font (note, font_str,
							TRUE);
				g_free (font_str);
			}

			/* Retrieve the workspace */
			{
				char *workspace_str;

				workspace_str = (gchar *)xmlGetProp (node, XML_CHAR ("workspace"));
				if (workspace_str)
				{
					note->workspace = atoi (workspace_str);
					g_free (workspace_str);
				}
			}

			/* Retrieve and set (if any) the body contents of the
			 * note */
			{
				gchar *body = (gchar *)xmlNodeListGetString(doc,
						node->xmlChildrenNode, 1);
				if (body) {
					GtkTextBuffer *buffer;
					GtkTextIter start, end;

					buffer = gtk_text_view_get_buffer(
						GTK_TEXT_VIEW(note->w_body));
					gtk_text_buffer_get_bounds(
							buffer, &start, &end);
					gtk_text_buffer_insert(buffer,
							&start, body, -1);
				}
				g_free(body);
			}

			/* Retrieve and set the locked state of the note,
			 * by default unlocked */
			{
				gchar *locked = (gchar *)xmlGetProp(node, XML_CHAR ("locked"));
				if (locked)
					stickynote_set_locked(note,
						!strcmp(locked, "true"));
				g_free(locked);
			}
		}

		node = node->next;
	}

	tmp1 = new_notes;
	/*
	wnck_screen = wnck_screen_get_default ();
	wnck_screen_force_update (wnck_screen);
	*/

	while (tmp1)
	{
		StickyNote *note = tmp1->data;

		stickynote_set_visible (note, stickynotes->visible);
		tmp1 = tmp1->next;
	}

	g_list_free (new_notes);
	g_list_free (new_nodes);

	xmlFreeDoc(doc);
}
