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

#ifndef __STICKYNOTES_APPLET_H__
#define __STICKYNOTES_APPLET_H__

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <mate-panel-applet.h>


#define STICKYNOTES_SCHEMA	"org.mate.stickynotes"
#define BUILDER_PATH	GTK_BUILDERDIR "/stickynotes.ui"

#define STICKYNOTES_STOCK_LOCKED	"stickynotes-stock-locked"
#define STICKYNOTES_STOCK_UNLOCKED	"stickynotes-stock-unlocked"
#define STICKYNOTES_STOCK_CLOSE 	"stickynotes-stock-close"
#define STICKYNOTES_STOCK_RESIZE_SE 	"stickynotes-stock-resize-se"
#define STICKYNOTES_STOCK_RESIZE_SW	"stickynotes-stock-resize-sw"

/* Global Sticky Notes instance */
typedef struct
{
	GtkBuilder *builder;		

	GtkWidget *w_prefs;		/* The prefs dialog */
	GtkAdjustment *w_prefs_width;
	GtkAdjustment *w_prefs_height;
	GtkWidget *w_prefs_color;
	GtkWidget *w_prefs_font_color;
	GtkWidget *w_prefs_sys_color;
	GtkWidget *w_prefs_font;
	GtkWidget *w_prefs_sys_font;
	GtkWidget *w_prefs_sticky;
	GtkWidget *w_prefs_force;
	GtkWidget *w_prefs_desktop;

	GList *notes;			/* Linked-List of all the sticky notes */
	GList *applets;			/* Linked-List of all the applets */
	
	GdkPixbuf *icon_normal;		/* Normal applet icon */
	GdkPixbuf *icon_prelight;	/* Prelighted applet icon */

	GSettings *settings;		/* Shared GSettings */

	gint max_height;
	guint last_timeout_data;

    gboolean visible;       /* Toggle show/hide notes */
} StickyNotes;

/* Sticky Notes Applet */
typedef struct
{
	GtkWidget *w_applet;		/* The applet */
	GtkWidget *w_image;		/* The applet icon */

	GtkWidget *destroy_all_dialog;	/* The applet it's destroy all dialog */
	
	gboolean prelighted;		/* Whether applet is prelighted */
	gboolean pressed;		/* Whether applet is pressed */

	gint panel_size;
	MatePanelAppletOrient panel_orient;

	GtkActionGroup *action_group;
	GtkWidget *menu_tip;
} StickyNotesApplet;
	
typedef enum
{
	STICKYNOTES_NEW = 0,
	STICKYNOTES_SET_VISIBLE,
	STICKYNOTES_SET_LOCKED

} StickyNotesDefaultAction;

extern StickyNotes *stickynotes;

void stickynotes_applet_init(MatePanelApplet *mate_panel_applet);
void stickynotes_applet_init_prefs(void);

StickyNotesApplet * stickynotes_applet_new(MatePanelApplet *mate_panel_applet);

void stickynotes_applet_update_icon(StickyNotesApplet *applet);
void stickynotes_applet_update_prefs(void);
void stickynotes_applet_update_menus(void);
void stickynotes_applet_update_tooltips(void);

void stickynotes_applet_do_default_action(GdkScreen *screen);

void stickynotes_applet_panel_icon_get_geometry (int *x, int *y, int *width, int *height);

#endif /* __STICKYNOTES_APPLET_H__ */
