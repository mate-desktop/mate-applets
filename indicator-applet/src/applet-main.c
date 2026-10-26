/*
A small wrapper utility to load indicators and put them as menu items
into the mate-panel using it's applet interface.

Copyright 2009-2010 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <glib/gi18n.h>
#include <mate-panel-applet.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#if HAVE_UBUNTU_INDICATOR

#define INDICATOR_SERVICE_APPMENU	"libappmenu.so"
#define INDICATOR_SERVICE_ME		"libme.so"
#define INDICATOR_SERVICE_DATETIME	"libdatetime.so"

#define INDICATOR_SERVICE_APPMENU_NG	"com.canonical.indicator.appmenu"
#define INDICATOR_SERVICE_ME_NG		"com.canonical.indicator.me"
#define INDICATOR_SERVICE_DATETIME_NG	"com.canonical.indicator.datetime"

#include <libindicator/indicator-object.h>
#endif

#if HAVE_AYATANA_INDICATOR

#define INDICATOR_SERVICE_APPMENU	"libayatana-appmenu.so"
#define INDICATOR_SERVICE_ME		"libayatana-me.so"
#define INDICATOR_SERVICE_DATETIME	"libayatana-datetime.so"

#define INDICATOR_SERVICE_APPMENU_NG	"org.ayatana.indicator.appmenu"
#define INDICATOR_SERVICE_ME_NG		"org.ayatana.indicator.me"
#define INDICATOR_SERVICE_DATETIME_NG	"org.ayatana.indicator.datetime"

#include <libayatana-indicator/indicator-object.h>
#endif

/* For new style indicators */

#if HAVE_UBUNTU_INDICATOR && HAVE_UBUNTU_INDICATOR_NG

#include <libido/libido.h>
#include <libindicator/indicator-ng.h>

#define INDICATOR_SERVICE_DIR "/usr/share/unity/indicators"

#endif

#if HAVE_AYATANA_INDICATOR && HAVE_AYATANA_INDICATOR_NG

#include <libayatana-ido/libayatana-ido.h>
#include <libayatana-indicator/indicator-ng.h>

#define INDICATOR_SERVICE_DIR "/usr/share/ayatana/indicators"

#endif

#include "tomboykeybinder.h"

static gchar * indicator_order[] = {
#if HAVE_UBUNTU_INDICATOR
	"libapplication.so",
	"libmessaging.so",
	"libsoundmenu.so",
	"libdatetime.so",
	"libsession.so",
#endif
#if HAVE_AYATANA_INDICATOR
	"libayatana-application.so",
	"libayatana-messaging.so",
	"libayatana-soundmenu.so",
	"libayatana-datetime.so",
	"libayatana-session.so",
#endif
	NULL
};

static GtkPackDirection packdirection;
static MatePanelAppletOrient orient;
static guint size;

#define  MENU_DATA_INDICATOR_OBJECT  "indicator-object"
#define  MENU_DATA_INDICATOR_ENTRY   "indicator-entry"

#define  IO_DATA_ORDER_NUMBER        "indicator-order-number"

static gboolean     applet_fill_cb (MatePanelApplet * applet, const gchar * iid, gpointer data);

static void update_accessible_desc (IndicatorObjectEntry * entry, GtkWidget * menuitem);

/*************
 * main
 * ***********/

#ifdef INDICATOR_APPLET
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("IndicatorAppletFactory",
               PANEL_TYPE_APPLET,
               "indicator-applet",
               applet_fill_cb, NULL);
#endif
#ifdef INDICATOR_APPLET_COMPLETE
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("IndicatorAppletCompleteFactory",
               PANEL_TYPE_APPLET,
               "indicator-applet-complete",
               applet_fill_cb, NULL);
#endif
#ifdef INDICATOR_APPLET_APPMENU
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("IndicatorAppletAppmenuFactory",
               PANEL_TYPE_APPLET,
               "indicator-applet-appmenu",
               applet_fill_cb, NULL);
#endif

/*************
 * log files
 * ***********/
#ifdef INDICATOR_APPLET
#define LOG_FILE_NAME  "indicator-applet.log"
#endif
#ifdef INDICATOR_APPLET_COMPLETE
#define LOG_FILE_NAME  "indicator-applet-complete.log"
#endif
#ifdef INDICATOR_APPLET_APPMENU
#define LOG_FILE_NAME  "indicator-applet-appmenu.log"
#endif
GOutputStream * log_file = NULL;

/*****************
 * Hotkey support
 * **************/
#ifdef INDICATOR_APPLET
gchar * hotkey_keycode = "<Super>M";
#endif
#ifdef INDICATOR_APPLET_SESSION
gchar * hotkey_keycode = "<Super>S";
#endif
#ifdef INDICATOR_APPLET_COMPLETE
gchar * hotkey_keycode = "<Super>S";
#endif
#ifdef INDICATOR_APPLET_APPMENU
gchar * hotkey_keycode = "<Super>F1";
#endif

/********************
 * Environment Names
 * *******************/
#ifdef INDICATOR_APPLET
#define INDICATOR_SPECIFIC_ENV  "indicator-applet-original"
#endif
#ifdef INDICATOR_APPLET_COMPLETE
#define INDICATOR_SPECIFIC_ENV  "indicator-applet-complete"
#endif
#ifdef INDICATOR_APPLET_APPMENU
#define INDICATOR_SPECIFIC_ENV  "indicator-applet-appmenu"
#endif

static const gchar * indicator_env[] = {
	"indicator-applet",
	INDICATOR_SPECIFIC_ENV,
	NULL
};

/*************
 * init function
 * ***********/

static gint
name2order (const gchar * name) {
	int i;

	for (i = 0; indicator_order[i] != NULL; i++) {
		if (g_strcmp0(name, indicator_order[i]) == 0) {
			return i;
		}
	}

	return -1;
}

typedef struct _incoming_position_t incoming_position_t;
struct _incoming_position_t {
	gint objposition;
	gint entryposition;
	gint menupos;
	gboolean found;
};

/* This function helps by determining where in the menu list
   this new entry should be placed.  It compares the objects
   that they're on, and then the individual entries.  Each
   is progressively more expensive. */
static void
place_in_menu (GtkWidget * widget, gpointer user_data)
{
	incoming_position_t * position = (incoming_position_t *)user_data;
	if (position->found) {
		/* We've already been placed, just finish the foreach */
		return;
	}

	IndicatorObject * io = INDICATOR_OBJECT(g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_OBJECT));
	g_assert(io != NULL);

	gint objposition = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(io), IO_DATA_ORDER_NUMBER));
	/* We've already passed it, well, then this is where
	   we should be be.  Stop! */
	if (objposition > position->objposition) {
		position->found = TRUE;
		return;
	}

	/* The objects don't match yet, keep looking */
	if (objposition < position->objposition) {
		position->menupos++;
		return;
	}

	/* The objects are the same, let's start looking at entries. */
	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_ENTRY);
	gint entryposition = indicator_object_get_location(io, entry);

	if (entryposition > position->entryposition) {
		position->found = TRUE;
		return;
	}

	if (entryposition < position->entryposition) {
		position->menupos++;
		return;
	}

	/* We've got the same object and the same entry.  Well,
	   let's just put it right here then. */
	position->found = TRUE;
	return;
}

static void
something_shown (GtkWidget * widget, gpointer user_data)
{
	GtkWidget * menuitem = GTK_WIDGET(user_data);
	gtk_widget_show(menuitem);
}

static void
something_hidden (GtkWidget * widget, gpointer user_data)
{
	GtkWidget * menuitem = GTK_WIDGET(user_data);
	gtk_widget_hide(menuitem);
}

static void
sensitive_cb (GObject * obj, GParamSpec * pspec, gpointer user_data)
{
	g_return_if_fail(GTK_IS_WIDGET(obj));
	g_return_if_fail(GTK_IS_WIDGET(user_data));

	gtk_widget_set_sensitive(GTK_WIDGET(user_data), gtk_widget_get_sensitive(GTK_WIDGET(obj)));
	return;
}

static void
entry_activated (GtkWidget * widget, gpointer user_data)
{
	g_return_if_fail(GTK_IS_WIDGET(widget));
	gpointer pio = g_object_get_data(G_OBJECT(widget), "indicator");
	g_return_if_fail(INDICATOR_IS_OBJECT(pio));
	IndicatorObject * io = INDICATOR_OBJECT(pio);

	return indicator_object_entry_activate(io, (IndicatorObjectEntry *)user_data, gtk_get_current_event_time());
}

static gboolean
entry_scrolled (GtkWidget *menuitem, GdkEventScroll *event, gpointer data)
{
	IndicatorObject *io = g_object_get_data (G_OBJECT (menuitem), MENU_DATA_INDICATOR_OBJECT);
	IndicatorObjectEntry *entry = g_object_get_data (G_OBJECT (menuitem), MENU_DATA_INDICATOR_ENTRY);

	g_return_val_if_fail(INDICATOR_IS_OBJECT(io), FALSE);

	g_signal_emit_by_name (io, "scroll", 1, event->direction);
	g_signal_emit_by_name (io, "scroll-entry", entry, 1, event->direction);
	g_signal_emit_by_name (io, INDICATOR_OBJECT_SIGNAL_ENTRY_SCROLLED, entry, 1, event->direction);

	return FALSE;
}

static gboolean
entry_pressed (GtkWidget *menuitem, GdkEvent *event, gpointer data)
{
       g_return_val_if_fail(GTK_IS_MENU_ITEM(menuitem), FALSE);

       if (((GdkEventButton*)event)->button == 2) /* middle button */
       {
               gtk_widget_grab_focus(menuitem);

               return TRUE;
       }

       return FALSE;
}

static gboolean
entry_released (GtkWidget *menuitem, GdkEvent *event, gpointer data)
{
       g_return_val_if_fail(GTK_IS_MENU_ITEM(menuitem), FALSE);

       if (((GdkEventButton*)event)->button == 2) /* middle button */
       {
               IndicatorObject *io = g_object_get_data (G_OBJECT (menuitem), MENU_DATA_INDICATOR_OBJECT);
               IndicatorObjectEntry *entry = g_object_get_data (G_OBJECT (menuitem), MENU_DATA_INDICATOR_ENTRY);

               g_return_val_if_fail(INDICATOR_IS_OBJECT(io), FALSE);

               g_signal_emit_by_name (io, INDICATOR_OBJECT_SIGNAL_SECONDARY_ACTIVATE, entry,
                       ((GdkEventButton*)event)->time);

               return TRUE;
       }

       return FALSE;
}

static void
accessible_desc_update_cb (GtkWidget * widget, gpointer userdata)
{
	gpointer data = g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_ENTRY);

	if (data != userdata) {
		return;
	}

	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)data;
	update_accessible_desc(entry, widget);
}

static void
accessible_desc_update (IndicatorObject * io, IndicatorObjectEntry * entry, GtkWidget * menubar)
{
	gtk_container_foreach(GTK_CONTAINER(menubar), accessible_desc_update_cb, entry);
	return;
}

#define PANEL_PADDING 8
static gboolean
entry_resized (GtkWidget *applet, guint newsize, gpointer data)
{
	IndicatorObject *io = (IndicatorObject *)data;

	size = newsize;

	/* Work on the entries */
	GList * entries = indicator_object_get_entries(io);
	GList * entry = NULL;

	for (entry = entries; entry != NULL; entry = g_list_next(entry)) {
		IndicatorObjectEntry * entrydata = (IndicatorObjectEntry *)entry->data;
		if (entrydata->image != NULL) {
			/* Resize to fit panel */
			gtk_image_set_pixel_size (entrydata->image, size - PANEL_PADDING);
		}
	}

	return FALSE;
}

static void
entry_added (IndicatorObject * io, IndicatorObjectEntry * entry, GtkWidget * menubar)
{
	g_debug("Signal: Entry Added");
	gboolean something_visible = FALSE;
	gboolean something_sensitive = FALSE;

	GtkWidget * menuitem = gtk_menu_item_new();
	GtkWidget * box = (packdirection == GTK_PACK_DIRECTION_LTR) ?
		gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3) : gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);

	/* Allows indicators to receive mouse scroll event */
	gtk_widget_add_events(GTK_WIDGET(menuitem), GDK_SCROLL_MASK);
	gtk_widget_add_events(GTK_WIDGET(menuitem), GDK_BUTTON_PRESS_MASK);
	gtk_widget_add_events(GTK_WIDGET(menuitem), GDK_BUTTON_RELEASE_MASK);

	g_object_set_data (G_OBJECT (menuitem), "indicator", io);
	g_object_set_data (G_OBJECT (menuitem), "box", box);

	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(entry_activated), entry);
	g_signal_connect(G_OBJECT(menuitem), "scroll-event", G_CALLBACK(entry_scrolled), entry);
	g_signal_connect(G_OBJECT(menuitem), "button-press-event", G_CALLBACK(entry_pressed), entry);
	g_signal_connect(G_OBJECT(menuitem), "button-release-event", G_CALLBACK(entry_released), entry);

	if (entry->image != NULL) {
		/* Resize to fit panel */
		gtk_image_set_pixel_size (entry->image, size - PANEL_PADDING);
		gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(entry->image), FALSE, FALSE, 1);
		if (gtk_widget_get_visible(GTK_WIDGET(entry->image))) {
			something_visible = TRUE;
		}

		if (gtk_widget_get_sensitive(GTK_WIDGET(entry->image))) {
			something_sensitive = TRUE;
		}

		g_signal_connect(G_OBJECT(entry->image), "show", G_CALLBACK(something_shown), menuitem);
		g_signal_connect(G_OBJECT(entry->image), "hide", G_CALLBACK(something_hidden), menuitem);

		g_signal_connect(G_OBJECT(entry->image), "notify::sensitive", G_CALLBACK(sensitive_cb), menuitem);
	}
	if (entry->label != NULL) {
		switch(packdirection) {
			case GTK_PACK_DIRECTION_LTR:
				gtk_label_set_angle(GTK_LABEL(entry->label), 0.0);
				break;
			case GTK_PACK_DIRECTION_TTB:
				gtk_label_set_angle(GTK_LABEL(entry->label),
						(orient == MATE_PANEL_APPLET_ORIENT_LEFT) ?
						270.0 : 90.0);
				break;
			default:
				break;
		}
		gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(entry->label), FALSE, FALSE, 1);

		if (gtk_widget_get_visible(GTK_WIDGET(entry->label))) {
			something_visible = TRUE;
		}

		if (gtk_widget_get_sensitive(GTK_WIDGET(entry->label))) {
			something_sensitive = TRUE;
		}

		g_signal_connect(G_OBJECT(entry->label), "show", G_CALLBACK(something_shown), menuitem);
		g_signal_connect(G_OBJECT(entry->label), "hide", G_CALLBACK(something_hidden), menuitem);

		g_signal_connect(G_OBJECT(entry->label), "notify::sensitive", G_CALLBACK(sensitive_cb), menuitem);
	}
	gtk_container_add(GTK_CONTAINER(menuitem), box);
	gtk_widget_show(box);

	if (entry->menu != NULL) {
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), GTK_WIDGET(entry->menu));
	}

	incoming_position_t position;
	position.objposition = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(io), IO_DATA_ORDER_NUMBER));
	position.entryposition = indicator_object_get_location(io, entry);
	position.menupos = 0;
	position.found = FALSE;

	gtk_container_foreach(GTK_CONTAINER(menubar), place_in_menu, &position);

	gtk_menu_shell_insert(GTK_MENU_SHELL(menubar), menuitem, position.menupos);

	if (something_visible) {
		if (entry->accessible_desc != NULL) {
			update_accessible_desc(entry, menuitem);
		}
		gtk_widget_show(menuitem);
	}
	gtk_widget_set_sensitive(menuitem, something_sensitive);

	g_object_set_data(G_OBJECT(menuitem), MENU_DATA_INDICATOR_ENTRY,  entry);
	g_object_set_data(G_OBJECT(menuitem), MENU_DATA_INDICATOR_OBJECT, io);

	return;
}

static void
entry_removed_cb (GtkWidget * widget, gpointer userdata)
{
	gpointer data = g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_ENTRY);

	if (data != userdata) {
		return;
	}

	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)data;
	if (entry->label != NULL) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->label), G_CALLBACK(something_shown), widget);
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->label), G_CALLBACK(something_hidden), widget);
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->label), G_CALLBACK(sensitive_cb), widget);
	}
	if (entry->image != NULL) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->image), G_CALLBACK(something_shown), widget);
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->image), G_CALLBACK(something_hidden), widget);
		g_signal_handlers_disconnect_by_func(G_OBJECT(entry->image), G_CALLBACK(sensitive_cb), widget);
	}

	gtk_widget_destroy(widget);
	return;
}

static void
entry_removed (IndicatorObject * io G_GNUC_UNUSED, IndicatorObjectEntry * entry,
               gpointer user_data)
{
	g_debug("Signal: Entry Removed");

	gtk_container_foreach(GTK_CONTAINER(user_data), entry_removed_cb, entry);

	return;
}

static void
entry_moved_find_cb (GtkWidget * widget, gpointer userdata)
{
	gpointer * array = (gpointer *)userdata;
	if (array[1] != NULL) {
		return;
	}

	gpointer data = g_object_get_data(G_OBJECT(widget), MENU_DATA_INDICATOR_ENTRY);

	if (data != array[0]) {
		return;
	}

	array[1] = widget;
	return;
}

/* Gets called when an entry for an object was moved. */
static void
entry_moved (IndicatorObject * io, IndicatorObjectEntry * entry,
             gint old G_GNUC_UNUSED, gint new G_GNUC_UNUSED, gpointer user_data)
{
	GtkWidget * menubar = GTK_WIDGET(user_data);

	gpointer array[2];
	array[0] = entry;
	array[1] = NULL;

	gtk_container_foreach(GTK_CONTAINER(menubar), entry_moved_find_cb, array);
	if (array[1] == NULL) {
		g_warning("Moving an entry that isn't in our menus.");
		return;
	}

	GtkWidget * mi = GTK_WIDGET(array[1]);
	g_object_ref(G_OBJECT(mi));
	gtk_container_remove(GTK_CONTAINER(menubar), mi);

	incoming_position_t position;
	position.objposition = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(io), IO_DATA_ORDER_NUMBER));
	position.entryposition = indicator_object_get_location(io, entry);
	position.menupos = 0;
	position.found = FALSE;

	gtk_container_foreach(GTK_CONTAINER(menubar), place_in_menu, &position);

	gtk_menu_shell_insert(GTK_MENU_SHELL(menubar), mi, position.menupos);
	g_object_unref(G_OBJECT(mi));

	return;
}

static void
menu_show (IndicatorObject * io, IndicatorObjectEntry * entry,
           guint32 timestamp, gpointer user_data)
{
	GtkWidget * menubar = GTK_WIDGET(user_data);

	if (entry == NULL) {
		/* Close any open menus instead of opening one */
		GList * entries = indicator_object_get_entries(io);
		GList * iterator = NULL;
		for (iterator = entries; iterator != NULL; iterator = g_list_next(iterator)) {
			IndicatorObjectEntry * entrydata = (IndicatorObjectEntry *)iterator->data;
			gtk_menu_popdown(entrydata->menu);
		}
		g_list_free(entries);

		/* And tell the menubar to exit activation mode too */
		gtk_menu_shell_cancel(GTK_MENU_SHELL(menubar));
		return;
	}

	// TODO: do something sensible here
}

static void
update_accessible_desc(IndicatorObjectEntry * entry, GtkWidget * menuitem)
{
	/* FIXME: We need to deal with the use case where the contents of the
	   label overrides what is found in the atk object's name, or at least
	   orca speaks the label instead of the atk object name.
	 */
	AtkObject * menuitem_obj = gtk_widget_get_accessible(menuitem);
	if (menuitem_obj == NULL) {
		/* Should there be an error printed here? */
		return;
	}

	if (entry->accessible_desc != NULL) {
		atk_object_set_name(menuitem_obj, entry->accessible_desc);
	} else {
		atk_object_set_name(menuitem_obj, "");
	}
	return;
}

static void
load_indicator (MatePanelApplet *applet, GtkWidget * menubar, IndicatorObject *io, const gchar *name)
{
	/* Set the environment it's in */
	indicator_object_set_environment(io, (const GStrv)indicator_env);

	/* Attach the 'name' to the object */
#if HAVE_AYATANA_INDICATOR_NG || HAVE_UBUNTU_INDICATOR_NG
	int pos = 5000 - indicator_object_get_position(io);
	if (pos > 5000) {
		pos = name2order(name);
	}
#else
	int pos = name2order(name);
#endif

	g_object_set_data(G_OBJECT(io), IO_DATA_ORDER_NUMBER, GINT_TO_POINTER(pos));

	/* Connect to its signals */
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED,   G_CALLBACK(entry_added),    menubar);
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED, G_CALLBACK(entry_removed),  menubar);
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ENTRY_MOVED,   G_CALLBACK(entry_moved),    menubar);
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_MENU_SHOW,     G_CALLBACK(menu_show),      menubar);
	g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE, G_CALLBACK(accessible_desc_update), menubar);

	/* Track panel resize */
	g_signal_connect_object(G_OBJECT(applet), "change-size", G_CALLBACK(entry_resized), G_OBJECT(io), 0);

	/* Work on the entries */
	GList * entries = indicator_object_get_entries(io);
	GList * entry = NULL;

	for (entry = entries; entry != NULL; entry = g_list_next(entry)) {
		IndicatorObjectEntry * entrydata = (IndicatorObjectEntry *)entry->data;
		entry_added(io, entrydata, menubar);
	}

	g_list_free(entries);
}

static gboolean
load_module (const gchar * name, MatePanelApplet *applet, GtkWidget * menubar)
{
	g_debug("Looking at Module: %s", name);
	g_return_val_if_fail(name != NULL, FALSE);

	if (!g_str_has_suffix(name, G_MODULE_SUFFIX)) {
		return FALSE;
	}

	g_debug("Loading Module: %s", name);

	/* Build the object for the module */
	gchar * fullpath = g_build_filename(INDICATOR_DIR, name, NULL);
	IndicatorObject * io = indicator_object_new_from_file(fullpath);
	g_free(fullpath);

	load_indicator(applet, menubar, io, name);

	return TRUE;
}

static void
load_modules (MatePanelApplet *applet, GtkWidget *menubar, gint *indicators_loaded)
{
	if (g_file_test(INDICATOR_DIR, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))) {
		GDir * dir = g_dir_open(INDICATOR_DIR, 0, NULL);

		const gchar * name;
		gint count = 0;
		while ((name = g_dir_read_name(dir)) != NULL) {
#ifdef INDICATOR_APPLET_APPMENU
			if (g_strcmp0(name, INDICATOR_SERVICE_APPMENU)) {
				continue;
			}
#else
			if (!g_strcmp0(name, INDICATOR_SERVICE_APPMENU)) {
				continue;
			}
#endif
#ifdef INDICATOR_APPLET
			if (!g_strcmp0(name, INDICATOR_SERVICE_ME)) {
				continue;
			}
			if (!g_strcmp0(name, INDICATOR_SERVICE_DATETIME)) {
				continue;
			}
#endif
			if (load_module(name, applet, menubar)) {
				count++;
			}
		}

		*indicators_loaded += count;

		g_dir_close (dir);
	}
}

#if HAVE_AYATANA_INDICATOR_NG || HAVE_UBUNTU_INDICATOR_NG

static void
load_indicators_from_indicator_files (MatePanelApplet *applet, GtkWidget *menubar, gint *indicators_loaded)
{
	GDir *dir;
	const gchar *name;
	GError *error = NULL;

	dir = g_dir_open (INDICATOR_SERVICE_DIR, 0, &error);

	if (!dir) {
		g_warning ("unable to open indicator service file directory: %s", error->message);
		g_error_free (error);

		return;
	}

	gint count = 0;
	while ((name = g_dir_read_name (dir))) {
		gchar *filename;
		IndicatorNg *indicator;

		filename = g_build_filename (INDICATOR_SERVICE_DIR, name, NULL);
		indicator = indicator_ng_new_for_profile (filename, "desktop", &error);
		g_free (filename);

#ifdef INDICATOR_APPLET_APPMENU
		if (g_strcmp0(name, INDICATOR_SERVICE_APPMENU_NG)) {
			continue;
		}
#else
		if (!g_strcmp0(name, INDICATOR_SERVICE_APPMENU_NG)) {
			continue;
		}
#endif
#ifdef INDICATOR_APPLET
		if (!g_strcmp0(name, INDICATOR_SERVICE_ME_NG)) {
			continue;
		}
		if (!g_strcmp0(name, INDICATOR_SERVICE_DATETIME_NG)) {
			continue;
		}
#endif

		if (indicator) {
			load_indicator(applet, menubar, INDICATOR_OBJECT (indicator), name);
			count++;
		}else{
			g_warning ("unable to load '%s': %s", name, error->message);
			g_clear_error (&error);
		}
	}

	*indicators_loaded += count;

	g_dir_close (dir);
}
#endif  /* HAVE_AYATANA_INDICATOR_NG || HAVE_UBUNTU_INDICATOR_NG */

static void
hotkey_filter (char * keystring G_GNUC_UNUSED, gpointer data)
{
	g_return_if_fail(GTK_IS_MENU_SHELL(data));

	/* Oh, wow, it's us! */
	GList * children = gtk_container_get_children(GTK_CONTAINER(data));
	if (children == NULL) {
		g_debug("Menubar has no children");
		return;
	}

	gtk_menu_shell_select_item(GTK_MENU_SHELL(data), GTK_WIDGET(g_list_last(children)->data));
	g_list_free(children);
	return;
}

static gboolean
menubar_press (GtkWidget * widget,
                    GdkEventButton *event,
                    gpointer data G_GNUC_UNUSED)
{
	if (event->button != 1) {
		g_signal_stop_emission_by_name(widget, "button-press-event");
	}

	return FALSE;
}

static gboolean
menubar_on_draw (GtkWidget * widget,
                 cairo_t * cr,
                 GtkWidget * menubar)
{
	/* FIXME: either port to gtk_render_focus or remove this function */
	if (gtk_widget_has_focus(menubar))
		gtk_paint_focus(gtk_widget_get_style(widget),
		                cr,
		                gtk_widget_get_state(menubar),
		                widget, "menubar-applet", 0, 0, -1, -1);

	return FALSE;
}

static void
about_cb (GtkAction *action G_GNUC_UNUSED,
          gpointer   data G_GNUC_UNUSED)
{
	static const gchar *authors[] = {
		"Ted Gould <ted@canonical.com>",
		NULL
	};

	static gchar *license[] = {
        N_("This program is free software: you can redistribute it and/or modify it "
           "under the terms of the GNU General Public License version 3, as published "
           "by the Free Software Foundation."),
        N_("This program is distributed in the hope that it will be useful, but "
           "WITHOUT ANY WARRANTY; without even the implied warranties of "
           "MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR "
           "PURPOSE.  See the GNU General Public License for more details."),
        N_("You should have received a copy of the GNU General Public License along "
           "with this program.  If not, see <http://www.gnu.org/licenses/>."),
		NULL
	};
	gchar *license_i18n;

	license_i18n = g_strconcat (_(license[0]), "\n\n", _(license[1]), "\n\n", _(license[2]), NULL);

	gtk_show_about_dialog(NULL,
		"version", VERSION,
		"copyright", _("Copyright \xc2\xa9 2009-2010 Canonical, Ltd.\n"
		               "Copyright \xc2\xa9 2011-2021 MATE developers"),
#ifdef INDICATOR_APPLET_APPMENU
		"comments", _("An applet to hold your application menus."),
#endif
		"comments", _("An applet to hold all of the system indicators."),
		"authors", authors,
		"license", license_i18n,
		"wrap-license", TRUE,
		"translator-credits", _("translator-credits"),
		"logo-icon-name", "mate-indicator-applet",
		"icon-name", "mate-indicator-applet",
		"website", "https://mate-desktop.org",
		"website-label", _("MATE Website"),
		NULL
	);

	g_free (license_i18n);

	return;
}

static gboolean
swap_orient_cb (GtkWidget *item, gpointer data)
{
	GtkWidget *from = (GtkWidget *) data;
	GtkWidget *to = (GtkWidget *) g_object_get_data(G_OBJECT(from), "to");
	g_object_ref(G_OBJECT(item));
	gtk_container_remove(GTK_CONTAINER(from), item);
	if (GTK_IS_LABEL(item)) {
			switch(packdirection) {
			case GTK_PACK_DIRECTION_LTR:
				gtk_label_set_angle(GTK_LABEL(item), 0.0);
				break;
			case GTK_PACK_DIRECTION_TTB:
				gtk_label_set_angle(GTK_LABEL(item),
						(orient == MATE_PANEL_APPLET_ORIENT_LEFT) ?
						270.0 : 90.0);
				break;
			default:
				break;
		}
	}
	gtk_box_pack_start(GTK_BOX(to), item, FALSE, FALSE, 0);
	return TRUE;
}

static gboolean
reorient_box_cb (GtkWidget *menuitem, gpointer data)
{
	GtkWidget *from = g_object_get_data(G_OBJECT(menuitem), "box");
	GtkWidget *to = (packdirection == GTK_PACK_DIRECTION_LTR) ?
		gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0) : gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	g_object_set_data(G_OBJECT(from), "to", to);
	gtk_container_foreach(GTK_CONTAINER(from), (GtkCallback)swap_orient_cb,
			from);
	gtk_container_remove(GTK_CONTAINER(menuitem), from);
	gtk_container_add(GTK_CONTAINER(menuitem), to);
	g_object_set_data(G_OBJECT(menuitem), "box", to);
	gtk_widget_show_all(menuitem);
	return TRUE;
}

static gboolean
matepanelapplet_reorient_cb (GtkWidget *applet, MatePanelAppletOrient neworient,
		gpointer data)
{
	GtkWidget *menubar = (GtkWidget *)data;
	if ((((neworient == MATE_PANEL_APPLET_ORIENT_UP) ||
			(neworient == MATE_PANEL_APPLET_ORIENT_DOWN)) &&
			((orient == MATE_PANEL_APPLET_ORIENT_LEFT) ||
			(orient == MATE_PANEL_APPLET_ORIENT_RIGHT))) ||
			(((neworient == MATE_PANEL_APPLET_ORIENT_LEFT) ||
			(neworient == MATE_PANEL_APPLET_ORIENT_RIGHT)) &&
			((orient == MATE_PANEL_APPLET_ORIENT_UP) ||
			(orient == MATE_PANEL_APPLET_ORIENT_DOWN)))) {
		packdirection = (packdirection == GTK_PACK_DIRECTION_LTR) ?
				GTK_PACK_DIRECTION_TTB : GTK_PACK_DIRECTION_LTR;
		gtk_menu_bar_set_pack_direction(GTK_MENU_BAR(menubar),
				packdirection);
		orient = neworient;
		gtk_container_foreach(GTK_CONTAINER(menubar),
				(GtkCallback)reorient_box_cb, NULL);
	}
	orient = neworient;
	return FALSE;
}

#ifdef N_
#undef N_
#endif
#define N_(x) x

static void
log_to_file_cb (GObject * source_obj G_GNUC_UNUSED,
                GAsyncResult * result G_GNUC_UNUSED, gpointer user_data)
{
	g_free(user_data);
	return;
}

static void
log_to_file (const gchar * domain G_GNUC_UNUSED,
             GLogLevelFlags level G_GNUC_UNUSED,
             const gchar * message,
             gpointer data G_GNUC_UNUSED)
{
	if (log_file == NULL) {
		GError * error = NULL;
		gchar * filename = g_build_filename(g_get_user_cache_dir(), LOG_FILE_NAME, NULL);
		GFile * file = g_file_new_for_path(filename);
		g_free(filename);

		if (!g_file_test(g_get_user_cache_dir(), G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
			GFile * cachedir = g_file_new_for_path(g_get_user_cache_dir());
			g_file_make_directory_with_parents(cachedir, NULL, &error);

			if (error != NULL) {
				g_error("Unable to make directory '%s' for log file: %s", g_get_user_cache_dir(), error->message);
			}
		}

		g_file_delete(file, NULL, NULL);

		GFileIOStream * io = g_file_create_readwrite(file,
		                          G_FILE_CREATE_REPLACE_DESTINATION, /* flags */
		                          NULL, /* cancelable */
		                          &error); /* error */
		if (error != NULL) {
			g_error("Unable to replace file: %s", error->message);
		}

		log_file = g_io_stream_get_output_stream(G_IO_STREAM(io));
	}

	gchar * outputstring = g_strdup_printf("%s\n", message);
	g_output_stream_write_async(log_file,
	                            outputstring, /* data */
	                            strlen(outputstring), /* length */
	                            G_PRIORITY_LOW, /* priority */
	                            NULL, /* cancelable */
	                            log_to_file_cb, /* callback */
	                            outputstring); /* data */

	return;
}

static gboolean
applet_fill_cb (MatePanelApplet * applet, const gchar * iid G_GNUC_UNUSED,
                gpointer data G_GNUC_UNUSED)
{
#if HAVE_AYATANA_INDICATOR_NG || HAVE_UBUNTU_INDICATOR_NG
	ido_init();
#endif

	static const GtkActionEntry menu_actions[] = {
		{"About", "help-about", N_("_About"), NULL, NULL, G_CALLBACK(about_cb)}
	};
	static const gchar *menu_xml = "<menuitem name=\"About\" action=\"About\"/>";

	static gboolean first_time = FALSE;
	GtkWidget *menubar;
	gint indicators_loaded = 0;
	GtkActionGroup *action_group;

	if (!first_time)
	{
		first_time = TRUE;
#ifdef INDICATOR_APPLET
		g_set_application_name(_("Indicator Applet"));
#endif
#ifdef INDICATOR_APPLET_COMPLETE
		g_set_application_name(_("Indicator Applet Complete"));
#endif
#ifdef INDICATOR_APPLET_APPMENU
		g_set_application_name(_("Indicator Applet Application Menu"));
#endif

		g_log_set_default_handler(log_to_file, NULL);

		tomboy_keybinder_init();
	}

	/* Set panel options */
	gtk_container_set_border_width(GTK_CONTAINER (applet), 0);
	mate_panel_applet_set_flags(applet, MATE_PANEL_APPLET_EXPAND_MINOR);
	menubar = gtk_menu_bar_new();
	action_group = gtk_action_group_new ("Indicator Applet Actions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group, menu_actions,
	                              G_N_ELEMENTS (menu_actions),
	                              menubar);
	mate_panel_applet_setup_menu(applet, menu_xml, action_group);
	g_object_unref(action_group);
#ifdef INDICATOR_APPLET
	atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (applet)),
	                     "indicator-applet");
#endif
#ifdef INDICATOR_APPLET_COMPLETE
	atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (applet)),
	                     "indicator-applet-complete");
#endif
#ifdef INDICATOR_APPLET_APPMENU
	atk_object_set_name (gtk_widget_get_accessible (GTK_WIDGET (applet)),
	                     "indicator-applet-appmenu");
#endif

	/* Init some theme/icon stuff */
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
	                                  INDICATOR_ICONS_DIR);
	/* g_debug("Icons directory: %s", INDICATOR_ICONS_DIR); */
	gtk_widget_set_name(GTK_WIDGET (applet), "fast-user-switch-applet");

	/* Build menubar */
	size = (mate_panel_applet_get_size (applet));
	orient = (mate_panel_applet_get_orient(applet));
	packdirection = ((orient == MATE_PANEL_APPLET_ORIENT_UP) ||
			(orient == MATE_PANEL_APPLET_ORIENT_DOWN)) ?
			GTK_PACK_DIRECTION_LTR : GTK_PACK_DIRECTION_TTB;
	gtk_menu_bar_set_pack_direction(GTK_MENU_BAR(menubar),
			packdirection);
	gtk_widget_set_can_focus (menubar, TRUE);
	gtk_widget_set_name(GTK_WIDGET (menubar), "fast-user-switch-menubar");
	g_signal_connect(menubar, "button-press-event", G_CALLBACK(menubar_press), NULL);
	g_signal_connect_after(menubar, "draw", G_CALLBACK(menubar_on_draw), menubar);
	g_signal_connect(applet, "change-orient",
			G_CALLBACK(matepanelapplet_reorient_cb), menubar);
	gtk_container_set_border_width(GTK_CONTAINER(menubar), 0);

	/* Add in filter func */
	tomboy_keybinder_bind(hotkey_keycode, hotkey_filter, menubar);

	load_modules(applet, menubar, &indicators_loaded);
#if HAVE_AYATANA_INDICATOR_NG || HAVE_UBUNTU_INDICATOR_NG
	load_indicators_from_indicator_files(applet, menubar, &indicators_loaded);
#endif

	if (indicators_loaded == 0) {
		/* A label to allow for click through */
		GtkWidget * item = gtk_label_new(_("No Indicators"));
		mate_panel_applet_set_background_widget(applet, item);
		gtk_container_add(GTK_CONTAINER(applet), item);
		gtk_widget_show(item);
	} else {
		gtk_container_add(GTK_CONTAINER(applet), menubar);
		mate_panel_applet_set_background_widget(applet, menubar);
		gtk_widget_show(menubar);
	}

	gtk_widget_show(GTK_WIDGET(applet));

	return TRUE;

}
