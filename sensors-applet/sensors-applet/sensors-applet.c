/*
 * Copyright (C) 2005-2009 Alex Murray <murray.alex@gmail.com>
 * Copyright (C) 2012-2021 MATE Developers
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

/** Contain the functions for operating on the SensorsApplet structure
 *  (represents the applet itself, and its associated variables.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include "sensors-applet.h"
#include "active-sensor.h"
#include "sensors-applet-settings.h"
#include "sensors-applet-plugins.h"

#ifdef HAVE_LIBNOTIFY
#include "active-sensor-libnotify.h"
#define DEFAULT_NOTIFY_TIMEOUT 3000
#endif

#include "prefs-dialog.h"
#include "about-dialog.h"

#define SENSORS_APPLET_MENU_FILE "SensorsApplet.xml"
/* initially set as sensors_applet->size to ensure a real value is stored */
#define DEFAULT_APPLET_SIZE 24
#define COLUMN_SPACING 2
#define ROW_SPACING 1

/* builder for sensor sorting verification */
static GVariantBuilder gvb_sensors_hash_list;

/* callbacks for panel menu */
static void prefs_cb(GtkAction *action, gpointer *data) {

    SensorsApplet *sensors_applet;
    sensors_applet = (SensorsApplet *)data;

    if (sensors_applet->prefs_dialog) {
        gtk_window_present(GTK_WINDOW(sensors_applet->prefs_dialog->dialog));
        return;
    }
    prefs_dialog_open(sensors_applet);
}

static void about_cb(GtkAction *action, gpointer data) {
    SensorsApplet *sensors_applet;
    sensors_applet = (SensorsApplet *)data;

    about_dialog_open(sensors_applet);
}

static void help_cb(GtkAction *action, gpointer data) {

    GError *error = NULL;

    gtk_show_uri_on_window(NULL, "help:mate-sensors-applet",

        gtk_get_current_event_time(),
        &error);

    if (error) {
        g_debug("Could not open help document: %s ",error->message);
        g_error_free(error);
    }
}

static void destroy_cb(GtkWidget *widget, gpointer data) {
    SensorsApplet *sensors_applet;
    sensors_applet = (SensorsApplet *)data;

    /* destroy dialogs, remove timeout and clear sensors tree and finally the applet */
    if (sensors_applet->prefs_dialog != NULL) {
        /* destroy's dialog too */
        prefs_dialog_close(sensors_applet);
    }

    if (sensors_applet->timeout_id) {
        g_source_remove(sensors_applet->timeout_id);
    }

    if (sensors_applet->settings) {
        g_object_unref (sensors_applet->settings);
        sensors_applet->settings = NULL;
    }

    /* destroy all active sensors */
    g_list_foreach(sensors_applet->active_sensors,
                   (GFunc)active_sensor_destroy,
                   NULL);

    if (sensors_applet->sensors != NULL) {
        gtk_tree_store_clear(sensors_applet->sensors);
    }

    gtk_widget_destroy(GTK_WIDGET(sensors_applet->applet));

    g_free(sensors_applet);
    return;
}

static void change_orient_cb (MatePanelApplet *applet,
                              MatePanelAppletOrient orient,
                              gpointer data) {

    SensorsApplet *sensors_applet;
    sensors_applet = (SensorsApplet *)data;

    sensors_applet_display_layout_changed(sensors_applet);
}

static void size_allocate_cb(MatePanelApplet *applet,
                             GtkAllocation *allocation,
                             gpointer data) {

    SensorsApplet *sensors_applet;
    MatePanelAppletOrient orient;

    sensors_applet = (SensorsApplet *)data;
    orient = mate_panel_applet_get_orient(sensors_applet->applet);

    if ((orient == MATE_PANEL_APPLET_ORIENT_LEFT) ||
        (orient == MATE_PANEL_APPLET_ORIENT_RIGHT)) {

        if (sensors_applet->size == allocation->width)
            return;
        sensors_applet->size = allocation->width;
    } else {
        if (sensors_applet->size == allocation->height)
            return;
        sensors_applet->size = allocation->height;
    }

    /* update if new value */
    sensors_applet_graph_size_changed(sensors_applet);
    sensors_applet_display_layout_changed(sensors_applet);
}

static void style_set_cb(GtkWidget *widget, GtkStyle *old_style, gpointer data) {

    /* update all icons in the sensors tree and update all active sensors */
    GtkTreeIter interfaces_iter, sensors_iter;
    gboolean not_end_of_interfaces = TRUE, not_end_of_sensors = TRUE;
    IconType icon_type;
    gboolean enabled;
    SensorsApplet *sensors_applet;
    DisplayMode display_mode;

    sensors_applet = (SensorsApplet *)data;

    display_mode = g_settings_get_int (sensors_applet->settings, DISPLAY_MODE);
    if (sensors_applet->sensors) {
        for (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter);
            not_end_of_interfaces;
            not_end_of_interfaces = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter)) {

            /* reset sensors sentinel */
            not_end_of_sensors = TRUE;

            for (gtk_tree_model_iter_children(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter, &interfaces_iter);
                not_end_of_sensors;
                not_end_of_sensors = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter)) {

                GdkPixbuf *new_icon;

                gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors),
                                   &sensors_iter,
                                   ENABLE_COLUMN, &enabled,
                                   ICON_TYPE_COLUMN, &icon_type,
                                   -1);
                /* update icons */
                new_icon = sensors_applet_load_icon(icon_type);

                gtk_tree_store_set(sensors_applet->sensors,
                                   &sensors_iter,
                                   ICON_PIXBUF_COLUMN, new_icon,
                                   -1);
                g_object_unref(new_icon);

                /* update icons only if currently being displayed */
                if (enabled &&
                    (display_mode == DISPLAY_ICON ||
                     display_mode == DISPLAY_ICON_WITH_VALUE)) {

                    GtkTreePath *path;

                    path = gtk_tree_model_get_path(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter);
                    sensors_applet_icon_changed(sensors_applet, path);
                    gtk_tree_path_free(path);
                }
            }
        }
        /* now update layout as size may have changed */
        sensors_applet_display_layout_changed(sensors_applet);
    }

}

static gboolean mouse_enter_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
    SensorsApplet *sensor_applet = data;
    sensor_applet->show_tooltip = TRUE;
    sensors_applet_update_active_sensors(sensor_applet);
    return TRUE;
}

static gboolean mouse_leave_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
    SensorsApplet *sensor_applet = data;
    sensor_applet->show_tooltip = FALSE;
    return TRUE;
}

static const GtkActionEntry sensors_applet_menu_actions[] = {
    { "Preferences", "document-properties", N_("_Preferences"),
        NULL, NULL,
        G_CALLBACK(prefs_cb) },
    { "Help", "help-browser", N_("_Help"),
        NULL, NULL,
        G_CALLBACK(help_cb) },
    { "About", "help-about", N_("_About"),
        NULL, NULL,
        G_CALLBACK(about_cb) }
};

void sensors_applet_notify_active_sensor(ActiveSensor *active_sensor, NotifType notif_type) {
#ifdef HAVE_LIBNOTIFY

    SensorsApplet *sensors_applet;
    gchar *summary, *message;
    gint timeout_msecs;
    gchar *sensor_label;
    gchar *sensor_path;
    SensorType sensor_type;
    TemperatureScale temp_scale;
    GtkTreeIter iter;
    GtkTreePath *path;
    const gchar *unit_type = NULL;
    const gchar *unit_type_title = NULL;
    const gchar *relation = NULL;
    const gchar *limit_type = NULL;
    const gchar *units = NULL;
    gdouble limit_value;
    gdouble seconds;
    gboolean show_notification = TRUE;

    sensors_applet = active_sensor->sensors_applet;

    if (!g_settings_get_boolean (sensors_applet->settings, DISPLAY_NOTIFICATIONS)) {
        g_debug("Wanted to display notification, but user has disabled them");
        return;
    }

    path = gtk_tree_row_reference_get_path(active_sensor->sensor_row);
    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(sensors_applet->sensors), &iter, path)) {

        gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors), &iter,
                           LABEL_COLUMN, &sensor_label,
                           PATH_COLUMN, &sensor_path,
                           SENSOR_TYPE_COLUMN, &sensor_type,
                           -1);
    } else {
        g_warning("Error getting data from tree for notification...");
        gtk_tree_path_free(path);
        return;
    }

    gtk_tree_path_free(path);

    /* do different stuff for different notif types */
    switch (notif_type) {
        case LOW_ALARM: /* fall thru */
        case HIGH_ALARM:
            if (active_sensor->sensor_values[0] <= active_sensor->sensor_low_value && notif_type == LOW_ALARM) {

                relation = _("is very low");
                limit_type = _("lower limit");
                limit_value = active_sensor->sensor_low_value;
            } else if (active_sensor->sensor_values[0] >= active_sensor->sensor_high_value && notif_type == HIGH_ALARM) {

                /* assume high alarm condition */
                relation = _("is very high");
                limit_type = _("upper limit");
                limit_value = active_sensor->sensor_high_value;
            } else {
                g_warning("Alarm notify called when no alarm condition!");
                g_free(sensor_path);
                g_free(sensor_label);
                return;
            }

            switch ((SensorType)sensor_type) {
                case TEMP_SENSOR:
                    unit_type_title = _("Temperature");
                    unit_type = _("temperature");
                    temp_scale = (TemperatureScale) g_settings_get_int (active_sensor->sensors_applet->settings, TEMPERATURE_SCALE);

                    switch (temp_scale) {
                        case CELSIUS:
                            units = UNITS_CELSIUS;
                            break;
                        case FAHRENHEIT:
                            units = UNITS_FAHRENHEIT;
                            break;
                        case KELVIN:
                            units = UNITS_KELVIN;
                            break;
                        default:
                            units = NULL;
                    }
                    break;

                case VOLTAGE_SENSOR:
                    unit_type_title = _("Voltage");
                    unit_type = _("voltage");
                    units = UNITS_VOLTAGE;
                    break;

                case FAN_SENSOR:
                    unit_type_title = _("Fan Speed");
                    unit_type = _("fan speed");
                    units = UNITS_RPM;
                    break;

                case CURRENT_SENSOR:
                    unit_type_title = _("Current");
                    unit_type = _("current");
                    units = UNITS_CURRENT;
                    break;
            }

            timeout_msecs = (active_sensor->alarm_timeout ? MIN(DEFAULT_NOTIFY_TIMEOUT, (active_sensor->alarm_timeout * 1000)) : DEFAULT_NOTIFY_TIMEOUT);

            summary = g_strdup_printf("%s %s %s", sensor_label, unit_type_title, _("Alarm"));
            message = g_strdup_printf("%s %s %s (%s %2.0f%s)", sensor_label, unit_type, relation, limit_type, limit_value, units);
            break;

        case SENSOR_INTERFACE_ERROR:
            /* get time since the last error */
            seconds = difftime(time(NULL), active_sensor->ierror_ts);

            /* if the last error happened less than 10 seconds ago, don't display this one
             * this should prevent recurring popups for removed sensors, like USB-HDDs */
            if (seconds < 11.0) {
                show_notification = FALSE;
            }

            summary = g_strdup_printf(_("Error updating sensor %s"), sensor_label);
            message = g_strdup_printf(_("An error occurred while trying to update the value of the sensor %s located at %s."), sensor_label, sensor_path);
            timeout_msecs = g_settings_get_int (active_sensor->sensors_applet->settings, TIMEOUT);

            /* update timestamp */
            time(&(active_sensor->ierror_ts));
            break;

        default:
            g_assert_not_reached();
    }

    if (show_notification) {
        active_sensor_libnotify_notify(active_sensor,
                                       notif_type,
                                       summary,
                                       message,
                                       "dialog-warning",
                                       timeout_msecs);
    }

    g_free(sensor_path);
    g_free(sensor_label);
    g_free(summary);
    g_free(message);
#endif
}

void sensors_applet_notify_end(ActiveSensor *active_sensor, NotifType notif_type) {
#ifdef HAVE_LIBNOTIFY
    active_sensor_libnotify_notify_end(active_sensor, notif_type);
#endif
}

#ifdef HAVE_LIBNOTIFY
static void sensors_applet_notify_end_all_gfunc(ActiveSensor *active_sensor, gpointer data) {
    active_sensor_libnotify_notify_end(active_sensor, LOW_ALARM);
    active_sensor_libnotify_notify_end(active_sensor, HIGH_ALARM);
}
#endif

void sensors_applet_notify_end_all(SensorsApplet *sensors_applet) {
#ifdef HAVE_LIBNOTIFY
    g_list_foreach(sensors_applet->active_sensors, (GFunc)sensors_applet_notify_end_all_gfunc, NULL);
#endif
}

/* internal helper functions for updating display etc */

/* should be called as a g_container_foreach at the start of
 * pack_display if the grid already exists to remove but keep alive
 * all children of the grid before repacking it */
static void sensors_applet_pack_display_empty_grid_cb(GtkWidget *widget, gpointer data)
{
    GtkContainer *container;

    container = GTK_CONTAINER(data);

    /* ref then remove widget */
    g_object_ref(widget);
    gtk_container_remove(container, widget);
}

/* should be called as a g_container_foreach at the end of
 * pack_display to unref any of the old children that we have readdded
 * to the table to stop reference creep from the g_object_ref called
 * on each child at the start of pack labels */
static void sensors_applet_pack_display_cleanup_refs_cb(GtkWidget *widget, gpointer data) {

    GList *old_children;

    old_children = (GList *)data;
    if (g_list_find(old_children, widget)) {
        g_object_unref(widget);
    }
}

static void sensors_applet_pack_display(SensorsApplet *sensors_applet) {
    /* note the if () around each widget is to ensure we only
     * operate on those that actually exist */
    GtkLabel *no_sensors_enabled_label = NULL;
    gint num_active_sensors = 0, num_sensors_per_group = 0, rows, cols, i, j;
    GList *old_grid_children = NULL;

    GList *current_sensor;

    DisplayMode display_mode;
    LayoutMode layout_mode;

    gboolean horizontal;
    gint label_width, icon_width, value_width;
    gint label_height, icon_height, value_height;

    GtkRequisition req;

    ActiveSensor *first_sensor;

    /* it is possible that there could be no active sensors so
     * handle that case first - make sure we dont do a NULL
     * pointer access first though */
    if (sensors_applet->active_sensors == NULL || g_list_length(sensors_applet->active_sensors) == 0) {
        g_debug("no active sensors to pack in grid");
        no_sensors_enabled_label = g_object_new(GTK_TYPE_LABEL, "label", _("No sensors enabled!"), NULL);

        if (sensors_applet->grid == NULL) {
            /* only need 1 row and 1 col */
            sensors_applet->grid = gtk_grid_new();
            gtk_grid_set_column_spacing(GTK_GRID(sensors_applet->grid), COLUMN_SPACING);
            gtk_grid_set_row_spacing(GTK_GRID(sensors_applet->grid), ROW_SPACING);
            gtk_widget_set_halign(sensors_applet->grid, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(sensors_applet->grid, GTK_ALIGN_CENTER);
            /* add grid to applet */
            gtk_container_add(GTK_CONTAINER(sensors_applet->applet), sensors_applet->grid);
        } else {
            /* destroy existing widgets - could be an
             * existing version of no sensors label - okay
             * to just add again though if destroy first */
            g_debug("destroying any existing widgets in container");
            gtk_container_foreach(GTK_CONTAINER(sensors_applet->grid), (GtkCallback)gtk_widget_destroy, NULL);
        }
        g_debug("packing no sensors enabled label");
        gtk_grid_attach(GTK_GRID(sensors_applet->grid),
                                 GTK_WIDGET(no_sensors_enabled_label),
                                 0, 0, 1, 1);
        gtk_widget_show_all(GTK_WIDGET(sensors_applet->applet));
        return;
    }
    /* otherwise can acess active_sensors without any worries */
    num_active_sensors = g_list_length(sensors_applet->active_sensors);

    display_mode = (DisplayMode) g_settings_get_int (sensors_applet->settings, DISPLAY_MODE);
    layout_mode = (LayoutMode) g_settings_get_int (sensors_applet->settings, LAYOUT_MODE);

    horizontal = (((mate_panel_applet_get_orient(sensors_applet->applet) == MATE_PANEL_APPLET_ORIENT_UP) ||
                  (mate_panel_applet_get_orient(sensors_applet->applet) == MATE_PANEL_APPLET_ORIENT_DOWN)));

    /* figure out num rows / cols by how high / wide sensors
     * labels / icons are and how much size we have to put them in */

    /* get the first active sensor */
    first_sensor = (ActiveSensor *)sensors_applet->active_sensors->data;

    switch (display_mode) {
        case DISPLAY_VALUE:
            gtk_widget_get_preferred_size(GTK_WIDGET(first_sensor->value), &req, NULL);
            value_width = req.width + COLUMN_SPACING;
            value_height = req.height + ROW_SPACING;

            /* make sure all widths and heights are non zero,
             * otherwise will get a divide by zero exception below
             * - is a non critical error since can happen when
             * elements first added to list, so simply return - is
             * not a programming error */
            if (value_width == 0 && value_height == 0) {
                return;
            }

            num_sensors_per_group = (sensors_applet->size / (horizontal ? value_height : value_width));
            break;

        case DISPLAY_LABEL_WITH_VALUE:
            /* even though we end up packing the event boxes into the
             * panel, these dont give back request sizes, so need to ask
             * widgets directly */
            gtk_widget_get_preferred_size(GTK_WIDGET(first_sensor->value), &req, NULL);

            value_width = req.width + COLUMN_SPACING;
            value_height = req.height + ROW_SPACING;

            gtk_widget_get_preferred_size(GTK_WIDGET(first_sensor->label), &req, NULL);
            label_width = req.width + COLUMN_SPACING;
            label_height = req.height + ROW_SPACING;

            /* make sure all widths and heights are non zero, otherwise
             * will get a divide by zero exception below
             * - is a non critical error since can happen when
             * elements first added to list, so simply return - is
             * not a programming error */
            if (!(label_width && label_height && value_width && value_height)) {
                return;
            }

            switch (layout_mode) {
                case VALUE_BESIDE_LABEL:
                    num_sensors_per_group = (sensors_applet->size / (horizontal ? MAX(label_height, value_height) : (label_width + value_width)));
                    break;
                case VALUE_BELOW_LABEL:
                    num_sensors_per_group = (sensors_applet->size / (horizontal ? (label_height + value_height) : MAX(label_width, value_width)));
                    break;
            }
            break;

        case DISPLAY_ICON_WITH_VALUE:
            gtk_widget_get_preferred_size(GTK_WIDGET(first_sensor->value), &req, NULL);
            value_width = req.width + COLUMN_SPACING;
            value_height = req.height + ROW_SPACING;

            gtk_widget_get_preferred_size(GTK_WIDGET(first_sensor->icon), &req, NULL);
            icon_width = req.width + COLUMN_SPACING;
            icon_height = req.height + ROW_SPACING;

            if (!(icon_width && icon_height && value_width && value_height)) {
                return;
            }

            switch (layout_mode) {
                case VALUE_BESIDE_LABEL:
                    num_sensors_per_group = (sensors_applet->size / (horizontal ? MAX(icon_height, value_height) : (icon_width + value_width)));
                    break;

                case VALUE_BELOW_LABEL:
                    num_sensors_per_group = (sensors_applet->size / (horizontal ? (icon_height + value_height) : MAX(icon_width, value_width)));
                    break;
            }
            break;

        case DISPLAY_ICON:
            gtk_widget_get_preferred_size(GTK_WIDGET(first_sensor->icon), &req, NULL);
            icon_width = req.width + COLUMN_SPACING;
            icon_height = req.height + ROW_SPACING;

            if (!(icon_width && icon_height)) {
                return;
            }

            num_sensors_per_group = (sensors_applet->size / (horizontal ? icon_height : icon_width));
            break;

        case DISPLAY_GRAPH:
            /* only show graphs in a line like System Monitor applet */
            num_sensors_per_group = 1;
            break;
    }

    /* ensure always atleast 1 sensor per group */
    if (num_sensors_per_group < 1) {
        /* force a better layout */
        if (horizontal && layout_mode == VALUE_BELOW_LABEL) {
            layout_mode = VALUE_BESIDE_LABEL;
        } else if (!horizontal && layout_mode == VALUE_BESIDE_LABEL) {
            layout_mode = VALUE_BELOW_LABEL;
        }
        num_sensors_per_group = 1;
    }

    if (horizontal) {
        /* if oriented horizontally, want as many
           sensors per column as user has defined, then
           enough columns to hold all the widgets */
        rows = num_sensors_per_group;
        cols = num_active_sensors / num_sensors_per_group;
        while (rows * cols < num_active_sensors || cols == 0) {
            cols++;
        }

    } else {
        /* if oriented vertically, want as many
           sensors per row as user has defined, then
           enough rows to hold all the widgets*/
        cols = num_sensors_per_group;
        rows = num_active_sensors / num_sensors_per_group;
        while (rows * cols < num_active_sensors || rows == 0) {
            rows++;
        }

    }

    /* if displaying labels / icons and values need to modify
       number of rows / colums to accomodate this */
     if (display_mode == DISPLAY_LABEL_WITH_VALUE || display_mode == DISPLAY_ICON_WITH_VALUE) {
         if (layout_mode == VALUE_BESIDE_LABEL) {
             /* to display labels next to values need twice as many columns */
             cols *= 2;
         } else {
             /* to display labels above values, we need twice as many rows as without */
             rows *= 2;
         }
     }

    if (sensors_applet->grid == NULL) {
        /* create grid and add to applet */
        sensors_applet->grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(sensors_applet->grid), COLUMN_SPACING);
        gtk_grid_set_row_spacing(GTK_GRID(sensors_applet->grid), ROW_SPACING);
        gtk_widget_set_halign(sensors_applet->grid, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(sensors_applet->grid, GTK_ALIGN_CENTER);
        gtk_container_add(GTK_CONTAINER(sensors_applet->applet), sensors_applet->grid);
    } else {
        /* remove all children if grid already exists so we can start again */
        /* save a list of the old children for later */
        old_grid_children = gtk_container_get_children(GTK_CONTAINER(sensors_applet->grid));

        gtk_container_foreach(GTK_CONTAINER(sensors_applet->grid),
                      sensors_applet_pack_display_empty_grid_cb,
                      sensors_applet->grid);
    }
    /* pack icons / labels and values into grid */
    current_sensor = sensors_applet->active_sensors;

    /* if showing labels / icons and values, need to pack labels / icons these first */
    if (display_mode == DISPLAY_ICON_WITH_VALUE || display_mode == DISPLAY_LABEL_WITH_VALUE) {
        /* loop through columns */
        for (i = 0; current_sensor != NULL && i < cols; /* increments depends on how we lay them out - see below */) {

            /* loop through rows in a column */
            for (j = 0; current_sensor && j < rows; /* see bottom of for loop*/) {
                /* attach label / icon at this point */
                if (display_mode == DISPLAY_ICON_WITH_VALUE) {
                    if (((ActiveSensor *)(current_sensor->data))->icon) {
                        gtk_grid_attach(GTK_GRID(sensors_applet->grid),
                                        ((ActiveSensor *)(current_sensor->data))->icon,
                                        i, j, 1, 1);
                    }
                } else {
                    if (((ActiveSensor *)(current_sensor->data))->label) {
                        gtk_grid_attach(GTK_GRID(sensors_applet->grid),
                                        ((ActiveSensor *)(current_sensor->data))->label,
                                        i, j, 1, 1);
                    }
                }
                /* now attach sensor value to either row below or column next to */
                if (layout_mode == VALUE_BESIDE_LABEL) {
                    /* left align labels */
                    if (((ActiveSensor *)(current_sensor->data))->icon) {
                        gtk_widget_set_halign (((ActiveSensor *)(current_sensor->data))->icon, GTK_ALIGN_START);
                        gtk_widget_set_valign (((ActiveSensor *)(current_sensor->data))->icon, GTK_ALIGN_CENTER);
                    }
                    if (((ActiveSensor *)(current_sensor->data))->label) {
                        gtk_label_set_xalign (GTK_LABEL(((ActiveSensor *)(current_sensor->data))->label), 0);
                        gtk_label_set_yalign (GTK_LABEL(((ActiveSensor *)(current_sensor->data))->label), 0.5);
                    }
                    if (((ActiveSensor *)(current_sensor->data))->value) {
                        gtk_widget_set_halign (((ActiveSensor *)(current_sensor->data))->value, GTK_ALIGN_START);
                        gtk_widget_set_valign (((ActiveSensor *)(current_sensor->data))->value, GTK_ALIGN_CENTER);
                    }

                     /* place value next to label */
                    if (((ActiveSensor *)(current_sensor->data))->value) {
                        gtk_grid_attach(GTK_GRID(sensors_applet->grid),
                                        ((ActiveSensor *)(current_sensor->data))->value,
                                        i + 1, j, 1, 1);
                    }
                    j++;
                } else { /* place value below label */

                    /* center align labels */
                    if (((ActiveSensor *)(current_sensor->data))->icon) {
                        gtk_widget_set_halign (((ActiveSensor *)(current_sensor->data))->icon, GTK_ALIGN_CENTER);
                        gtk_widget_set_valign (((ActiveSensor *)(current_sensor->data))->icon, GTK_ALIGN_CENTER);
                    }
                    if (((ActiveSensor *)(current_sensor->data))->label) {
                        gtk_label_set_xalign (GTK_LABEL(((ActiveSensor *)(current_sensor->data))->label), 0.5);
                        gtk_label_set_yalign (GTK_LABEL(((ActiveSensor *)(current_sensor->data))->label), 0.5);
                    }
                    if (((ActiveSensor *)(current_sensor->data))->value) {
                        gtk_widget_set_halign (((ActiveSensor *)(current_sensor->data))->value, GTK_ALIGN_CENTER);
                        gtk_widget_set_valign (((ActiveSensor *)(current_sensor->data))->value, GTK_ALIGN_CENTER);
                    }

                    if (((ActiveSensor *)(current_sensor->data))->value) {
                        gtk_grid_attach(GTK_GRID(sensors_applet->grid),
                                        ((ActiveSensor *)(current_sensor->data))->value,
                                        i, j + 1, 1, 1);
                    }
                    j += 2;
                }
                current_sensor = g_list_next(current_sensor);
            } /* end row loop */

            /* now increment column index as needed */
            if (layout_mode == VALUE_BESIDE_LABEL) { /* place value next to label */
                i += 2;
            } else {
                i++;
            }
        } /* end column loop    */

    } else { /* not showing labels and icons with values, so just pack either only icons or values */
        for (i = 0; current_sensor != NULL && i < cols; ++i) {
            for (j = 0; current_sensor!= NULL && j < rows; ++j) {
                if (display_mode == DISPLAY_VALUE) {
                    if (((ActiveSensor *)(current_sensor->data))->value) {
                        gtk_grid_attach(GTK_GRID(sensors_applet->grid),
                                        ((ActiveSensor *)(current_sensor->data))->value,
                                        i, j, 1, 1);
                    }
                } else if (display_mode == DISPLAY_ICON) {
                    if (((ActiveSensor *)(current_sensor->data))->value) {
                        gtk_grid_attach(GTK_GRID(sensors_applet->grid),
                                        ((ActiveSensor *)(current_sensor->data))->icon,
                                        i, j, 1, 1);
                    }
                } else if (display_mode == DISPLAY_GRAPH) {
                    if (((ActiveSensor *)(current_sensor->data))->graph) {
                        gtk_grid_attach(GTK_GRID(sensors_applet->grid),
                                        ((ActiveSensor *)(current_sensor->data))->graph_frame,
                                        i, j, 1, 1);
                    }
                }

                current_sensor = g_list_next(current_sensor);
            }
        }
    }

    if (old_grid_children != NULL) {
        gtk_container_foreach(GTK_CONTAINER(sensors_applet->grid),
                      sensors_applet_pack_display_cleanup_refs_cb,
                      old_grid_children);
        g_list_free(old_grid_children);
    }
    gtk_widget_show_all(GTK_WIDGET(sensors_applet->applet));

}

/* must unref when done with returned pixbuf */
GdkPixbuf *sensors_applet_load_icon(IconType icon_type) {
    GtkIconTheme *icon_theme;
    GdkPixbuf *icon = NULL;
    GError *error = NULL;

    /* try to load the icon */

    /* not allowed to unref or ref icon_theme once we have it */
    icon_theme = gtk_icon_theme_get_default();
    icon = gtk_icon_theme_load_icon(icon_theme,
                                    stock_icons[icon_type],
                                    DEFAULT_ICON_SIZE,
                                    GTK_ICON_LOOKUP_USE_BUILTIN,
                                    &error);

    if (error) {
        g_warning ("Could not load icon: %s", error->message);
        g_error_free(error);
        error = NULL;

        /* try again with default icon */
        icon = gtk_icon_theme_load_icon(icon_theme,
                                        "image-missing",
                                        DEFAULT_ICON_SIZE,
                                        GTK_ICON_LOOKUP_USE_BUILTIN,
                                        &error);

        if (error) {
            /* this will quit sensors-applet but it is a pretty major error so may as well */
            g_error("Could not load GTK_STOCK_MISSING_IMAGE - major error!!!: %s", error->message);
            g_error_free(error);
            error = NULL;
        }

    }
    return icon;
}

// MUST FREE STRINGS AFTER CALLING THIS FUNCTION!!
gboolean sensors_applet_add_sensor(SensorsApplet *sensors_applet,
                                   const gchar *path,
                                   const gchar *id,
                                   const gchar *label,
                                   const gchar *interface,
                                   SensorType type,
                                   gboolean enable,
                                   gdouble low_value,
                                   gdouble high_value,
                                   gboolean alarm_enable,
                                   const gchar *low_alarm_command,
                                   const gchar *high_alarm_command,
                                   gint alarm_timeout,
                                   gdouble multiplier,
                                   gdouble offset,
                                   IconType icon_type,
                                   const gchar *graph_color) {

    GtkTreeIter interfaces_iter, sensors_iter;
    gboolean not_empty_tree;

    gchar *node_interface;
    gboolean not_end_of_interfaces = TRUE, interface_exists = FALSE;
    gboolean not_end_of_sensors = TRUE;
    gchar *sensor_id;
    gchar *sensor_path;
    SensorType sensor_type;
    GdkPixbuf *icon;
    GtkTreePath *tree_path;

    g_assert(sensors_applet);

    /* assume tree is not empty */
    not_empty_tree = TRUE;

    if (NULL == sensors_applet->sensors) {

        sensors_applet->sensors = gtk_tree_store_new(N_COLUMNS,
                                 G_TYPE_STRING, /* path */
                                 G_TYPE_STRING, /* id */
                                 G_TYPE_STRING, /* label */
                                 G_TYPE_STRING, /* interface */
                                 G_TYPE_UINT, /* sensor type */
                                 G_TYPE_BOOLEAN, /* enable */
                                 G_TYPE_BOOLEAN, /* visible */
                                 G_TYPE_DOUBLE, /* low value */
                                 G_TYPE_DOUBLE, /* high type */
                                 G_TYPE_BOOLEAN, /* alarm enable */
                                 G_TYPE_STRING, /* low alarm command */
                                 G_TYPE_STRING, /* high alarm command */
                                 G_TYPE_UINT, /* alarm timeout */
                                 G_TYPE_DOUBLE, /* multiplier */
                                 G_TYPE_DOUBLE, /* offset */
                                 G_TYPE_UINT, /* icon type */
                                 GDK_TYPE_PIXBUF, /* icon pixbuf */
                                 G_TYPE_STRING); /* graph color */

        g_debug("Sensor tree created.");

        /* we know tree is actually empty since we just created it */
        not_empty_tree = FALSE;
    }

    /* search sensor tree for the parent interface to place this sensor under */
    for (not_empty_tree = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter);
        not_empty_tree && not_end_of_interfaces && !interface_exists;
        not_end_of_interfaces = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter)) {

        gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter, INTERFACE_COLUMN, &node_interface, -1);
        if (g_ascii_strcasecmp(interface, node_interface) == 0) {
            /* found interface in tree */
            interface_exists = TRUE;

            /* now see if this actual sensor already exists within this interface - don't want to add duplicates */
            /* see if have children */
            for (not_end_of_sensors = gtk_tree_model_iter_children(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter,  &interfaces_iter);
                not_end_of_sensors;
                not_end_of_sensors = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter)) {

                gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter,
                           PATH_COLUMN, &sensor_path,
                           ID_COLUMN, &sensor_id,
                           SENSOR_TYPE_COLUMN, &sensor_type,
                           -1);
                if (g_ascii_strcasecmp(sensor_id, id) == 0 && g_ascii_strcasecmp(sensor_path, path) == 0  && sensor_type == type) {
                    /* sensor already exists so dont add a second time */
                    g_debug("sensor with path: %s, id: %s already exists in tree, not adding a second time", sensor_path, sensor_id);
                    g_free(sensor_id);
                    g_free(sensor_path);
                    g_free(node_interface);
                    return FALSE;
                }
                g_free(sensor_id);
                g_free(sensor_path);
            }
            g_free(node_interface);
            break;
        }
        g_free(node_interface);
    }

    if (!interface_exists) {
        /* add to required plugins hash table so we ensure this
           plugin stays loaded to make sure we have a get sensor
           value function if possible */
        g_hash_table_insert(sensors_applet->required_plugins,
                            g_strdup(interface),
                            GINT_TO_POINTER(TRUE));
        g_debug("added interface %s to required plugins", interface);

        /* wasn't able to find interface root node so create it */
        gtk_tree_store_append(sensors_applet->sensors,
                      &interfaces_iter,
                      NULL);

        gtk_tree_store_set(sensors_applet->sensors,
                   &interfaces_iter,
                   ID_COLUMN, interface,
                   INTERFACE_COLUMN, interface,
                   VISIBLE_COLUMN, FALSE,
                   -1);
        g_debug("Added sensor interface %s to tree", interface);
    }

    /* then add sensor as a child under interface node - ie assume
     * we either found it or created it - the inteface node that is */

    /* for now just add sensors all in a single list */
    gtk_tree_store_append(sensors_applet->sensors, &sensors_iter, &interfaces_iter);

    /* if sensor is already in settings, load values from there */
    gchar *applet_path = mate_panel_applet_get_preferences_path (sensors_applet->applet);
    gchar *gsuid = sensors_applet_settings_get_unique_id (interface, id, path);
    gchar *settings_path = g_strdup_printf ("%s%s/", applet_path, gsuid);
    GSettings *settings = g_settings_new_with_path ("org.mate.sensors-applet.sensor", settings_path);

    /* add hash to temp sorting list */
    g_variant_builder_add (&gvb_sensors_hash_list, "s", gsuid);

    g_free (applet_path);
    g_free (gsuid);
    g_free (settings_path);

    gchar *settings_id = g_settings_get_string (settings, ID);

    if (settings_id != NULL && settings_id[0] != '\0') {
        enable = g_settings_get_boolean (settings, ENABLED);
        icon = sensors_applet_load_icon(g_settings_get_int (settings, ICON_TYPE));
        gtk_tree_store_set(sensors_applet->sensors,
                           &sensors_iter,
                           PATH_COLUMN, g_settings_get_string (settings, PATH),
                           ID_COLUMN, settings_id,
                           LABEL_COLUMN, g_settings_get_string (settings, LABEL),
                           INTERFACE_COLUMN, g_settings_get_string (settings, INTERFACE),
                           SENSOR_TYPE_COLUMN, g_settings_get_int (settings, SENSOR_TYPE),
                           ENABLE_COLUMN, enable,
                           VISIBLE_COLUMN, TRUE,
                           LOW_VALUE_COLUMN, g_settings_get_double (settings, LOW_VALUE),
                           HIGH_VALUE_COLUMN, g_settings_get_double (settings, HIGH_VALUE),
                           ALARM_ENABLE_COLUMN, g_settings_get_boolean (settings, ALARM_ENABLED),
                           ALARM_TIMEOUT_COLUMN, g_settings_get_int (settings, ALARM_TIMEOUT),
                           LOW_ALARM_COMMAND_COLUMN, g_settings_get_string (settings, LOW_ALARM_COMMAND),
                           HIGH_ALARM_COMMAND_COLUMN, g_settings_get_string (settings, HIGH_ALARM_COMMAND),
                           MULTIPLIER_COLUMN, g_settings_get_double (settings, MULTIPLIER),
                           OFFSET_COLUMN, g_settings_get_double (settings, OFFSET),
                           ICON_TYPE_COLUMN, g_settings_get_int (settings, ICON_TYPE),
                           ICON_PIXBUF_COLUMN, icon,
                           GRAPH_COLOR_COLUMN, g_settings_get_string (settings, GRAPH_COLOR),
                           -1);
        g_free (settings_id);
    } else {
        icon = sensors_applet_load_icon(icon_type);
        gtk_tree_store_set(sensors_applet->sensors,
                           &sensors_iter,
                           PATH_COLUMN, path,
                           ID_COLUMN, id,
                           LABEL_COLUMN, label,
                           INTERFACE_COLUMN, interface,
                           SENSOR_TYPE_COLUMN, type,
                           ENABLE_COLUMN, enable,
                           VISIBLE_COLUMN, TRUE,
                           LOW_VALUE_COLUMN, low_value,
                           HIGH_VALUE_COLUMN, high_value,
                           ALARM_ENABLE_COLUMN, alarm_enable,
                           ALARM_TIMEOUT_COLUMN, alarm_timeout,
                           LOW_ALARM_COMMAND_COLUMN, low_alarm_command,
                           HIGH_ALARM_COMMAND_COLUMN, high_alarm_command,
                           MULTIPLIER_COLUMN, multiplier,
                           OFFSET_COLUMN, offset,
                           ICON_TYPE_COLUMN, icon_type,
                           ICON_PIXBUF_COLUMN, icon,
                           GRAPH_COLOR_COLUMN, graph_color,
                           -1);
    }
    g_object_unref (settings);
    g_debug("added sensor %s to tree", path);

    /* remove reference to icon as tree now has ref */
    g_object_unref(icon);

    /* create the active sensor */
    if (enable) {
        tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter);
        sensors_applet_sensor_enabled(sensors_applet, tree_path);
        gtk_tree_path_free(tree_path);
    }
    return TRUE;
}

static ActiveSensor *sensors_applet_find_active_sensor(SensorsApplet *sensors_applet, GtkTreePath *path) {
    GtkTreePath *sensor_tree_path;
    GList *current_sensor;

    for (current_sensor = sensors_applet->active_sensors; current_sensor != NULL; current_sensor = g_list_next(current_sensor)) {
        sensor_tree_path = gtk_tree_row_reference_get_path(((ActiveSensor *)(current_sensor->data))->sensor_row);

        if (gtk_tree_path_compare(path, sensor_tree_path) == 0) {
            gtk_tree_path_free(sensor_tree_path);
            return ((ActiveSensor *)(current_sensor->data));
        }
        gtk_tree_path_free(sensor_tree_path);
    }
    return NULL;
}

/* path should be the full path to a file representing the sensor (eg
 * /dev/hda or /sys/devices/platform/i2c-0/0-0290/temp1_input) */
void sensors_applet_display_layout_changed(SensorsApplet *sensors_applet) {
    /* update sensors since will need to update icons / graphs etc
     * if weren't displayed before */
    GList *list = NULL;
    for (list = sensors_applet->active_sensors; list != NULL; list = list->next) {
        ActiveSensor *as = (ActiveSensor *)list->data;
        as->updated = FALSE;
    }
    sensors_applet_update_active_sensors(sensors_applet);
    sensors_applet_pack_display(sensors_applet);
}

void sensors_applet_alarm_off(SensorsApplet *sensors_applet,
                              GtkTreePath *path,
                              NotifType notif_type) {
    ActiveSensor *active_sensor;

    if ((active_sensor = sensors_applet_find_active_sensor(sensors_applet, path)) != NULL) {
        active_sensor_alarm_off(active_sensor, notif_type);
    }
}

void sensors_applet_all_alarms_off(SensorsApplet *sensors_applet, GtkTreePath *path) {
    sensors_applet_alarm_off(sensors_applet, path, LOW_ALARM);
    sensors_applet_alarm_off(sensors_applet, path, HIGH_ALARM);
}

void sensors_applet_sensor_enabled(SensorsApplet *sensors_applet, GtkTreePath *path) {
    ActiveSensor *active_sensor;

    g_assert(sensors_applet);
    g_assert(path);

    active_sensor = active_sensor_new(sensors_applet, gtk_tree_row_reference_new(GTK_TREE_MODEL(sensors_applet->sensors), path));
    active_sensor_update(active_sensor, sensors_applet);

    /* keep list sorted */
    sensors_applet->active_sensors = g_list_insert_sorted(sensors_applet->active_sensors,
                                                              active_sensor,
                                                              (GCompareFunc)active_sensor_compare);

    sensors_applet_pack_display(sensors_applet);
}

void sensors_applet_reorder_sensors(SensorsApplet *sensors_applet) {
    sensors_applet->active_sensors = g_list_sort(sensors_applet->active_sensors, (GCompareFunc)active_sensor_compare);

    sensors_applet_pack_display(sensors_applet);
}

void sensors_applet_sensor_disabled(SensorsApplet *sensors_applet, GtkTreePath *path) {

    ActiveSensor *active_sensor;

    g_assert(sensors_applet);
    g_assert(path);

    if ((active_sensor = sensors_applet_find_active_sensor(sensors_applet, path)) != NULL) {
        g_debug("Destroying active sensor...");

        g_debug("-- removing from list...");
        sensors_applet->active_sensors = g_list_remove(sensors_applet->active_sensors, active_sensor);
        g_debug("-- repacking display....");
        sensors_applet_pack_display(sensors_applet);

        active_sensor_destroy(active_sensor);
    }
}

void sensors_applet_update_sensor(SensorsApplet *sensors_applet, GtkTreePath *path) {
    ActiveSensor *active_sensor;

    g_assert(sensors_applet);
    g_assert(path);

    if ((active_sensor = sensors_applet_find_active_sensor(sensors_applet, path)) != NULL) {
        active_sensor_update(active_sensor, sensors_applet);
    }
}

void sensors_applet_icon_changed(SensorsApplet *sensors_applet, GtkTreePath *path) {
    ActiveSensor *active_sensor;

    g_assert(sensors_applet);
    g_assert(path);

    if ((active_sensor = sensors_applet_find_active_sensor(sensors_applet, path)) != NULL) {
        active_sensor_icon_changed(active_sensor, sensors_applet);
    }
}

/* Cycle thru ActiveSensors and update them all */
gboolean sensors_applet_update_active_sensors(SensorsApplet *sensors_applet) {
    g_assert(sensors_applet);

    if (sensors_applet->active_sensors) {
        g_list_foreach(sensors_applet->active_sensors,
                       (GFunc)active_sensor_update,
                       sensors_applet);
        return TRUE;
    }
    return FALSE;
}

/* Cycle thru ActiveSensors and set new graph dimensions */
void sensors_applet_graph_size_changed(SensorsApplet *sensors_applet) {
    gint dimensions[2];
    gint graph_size;
    g_assert(sensors_applet);

    if (sensors_applet->active_sensors) {

        graph_size = g_settings_get_int (sensors_applet->settings, GRAPH_SIZE);
        if (mate_panel_applet_get_orient(sensors_applet->applet) ==  MATE_PANEL_APPLET_ORIENT_UP ||
            mate_panel_applet_get_orient(sensors_applet->applet) == MATE_PANEL_APPLET_ORIENT_DOWN) {

            /* is horizontal so set graph_size as width */
            dimensions[0] = graph_size;
            dimensions[1] = sensors_applet->size;
        } else {
            dimensions[0] = sensors_applet->size;
            dimensions[1] = graph_size;
        }

        g_list_foreach(sensors_applet->active_sensors,
                       (GFunc)active_sensor_update_graph_dimensions,
                       &dimensions);
    }

}

gdouble sensors_applet_convert_temperature(gdouble value, TemperatureScale old, TemperatureScale new) {

    switch (old) {
        case KELVIN:
            switch (new) {
                case CELSIUS:
                    value = value - 273.0;
                    break;
                case FAHRENHEIT:
                    value = (9.0 * (value - 273) / 5.0) + 32.0;
                    break;
                case KELVIN:
                    break;
            }
            break;

        case CELSIUS:
            switch (new) {
                case FAHRENHEIT:
                    value = (9.0 * value / 5.0) + 32.0;
                    break;
                case KELVIN:
                    value = value + 273.0;
                    break;
                case CELSIUS:
                    break;
            }
            break;

        case FAHRENHEIT:
            switch (new) {
            case CELSIUS:
                value = (5.0 * (value - 32.0) / 9.0);
                break;
            case KELVIN:
                value = (5.0 * (value - 32.0) / 9.0) + 273.0;
                break;
            case FAHRENHEIT:
                break;
            }
            break;
    }
    return value;
}

void sensors_applet_init(SensorsApplet *sensors_applet) {

    g_assert(sensors_applet);
    g_assert(sensors_applet->applet);

    GtkActionGroup *action_group;
    gchar *ui_path;

    /* Have our background automatically painted. */
    mate_panel_applet_set_background_widget(MATE_PANEL_APPLET(sensors_applet->applet), GTK_WIDGET(sensors_applet->applet));

    /* plugin functions are stored as name -> get_value_function pairs so
     * use standard string functions on hash table */
    sensors_applet->plugins = g_hash_table_new(g_str_hash, g_str_equal);

    sensors_applet->required_plugins = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    /* initialise size */
    sensors_applet->size = DEFAULT_APPLET_SIZE;

    mate_panel_applet_set_flags(sensors_applet->applet, MATE_PANEL_APPLET_EXPAND_MINOR);

    g_signal_connect(sensors_applet->applet, "destroy",
             G_CALLBACK(destroy_cb),
             sensors_applet);

    /* init gsettings */
    sensors_applet->settings = mate_panel_applet_settings_new (sensors_applet->applet, "org.mate.sensors-applet");

    /* set up builder for sorting verification */
    g_variant_builder_init (&gvb_sensors_hash_list, G_VARIANT_TYPE ("as"));

    /* set up / load sensors from the plugins */
    sensors_applet_plugins_load_all(sensors_applet);

    /* set sorting hash array */
    GVariant *gv_temp = g_variant_builder_end (&gvb_sensors_hash_list);
    sensors_applet->sensors_hash_array = g_variant_dup_strv (gv_temp, NULL);
    g_variant_unref (gv_temp);

    /* sort sensors based on saved sorting */
    sensors_applet_settings_sort_sensors(sensors_applet);

    /* free hash array */
    g_strfreev (sensors_applet->sensors_hash_array);

    /* should have created sensors tree above, but if have not was because we couldn't find any sensors */
    if (NULL == sensors_applet->sensors) {
        GtkWidget *label;
        label = gtk_label_new(_("No sensors found!"));
        gtk_container_add(GTK_CONTAINER(sensors_applet->applet), label);
        gtk_widget_show_all(GTK_WIDGET(sensors_applet->applet));
        return;
    }

    /* only do menu and signal connections if sensors are found */
    action_group = gtk_action_group_new ("Sensors Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group,
                                sensors_applet_menu_actions,
                                G_N_ELEMENTS (sensors_applet_menu_actions),
                                sensors_applet);
    ui_path = g_build_filename (UIDIR, SENSORS_APPLET_MENU_FILE, NULL);
    mate_panel_applet_setup_menu_from_file (sensors_applet->applet, ui_path, action_group);
    g_free (ui_path);
    g_object_unref (action_group);

    g_signal_connect(sensors_applet->applet, "style-set",
                     G_CALLBACK(style_set_cb),
                     sensors_applet);

    g_signal_connect(G_OBJECT(sensors_applet->applet), "change_orient",
                      G_CALLBACK(change_orient_cb),
                      sensors_applet);

    g_signal_connect(G_OBJECT(sensors_applet->applet), "size_allocate",
                      G_CALLBACK(size_allocate_cb),
                      sensors_applet);

    g_signal_connect(G_OBJECT(sensors_applet->applet), "leave_notify_event",
                     G_CALLBACK(mouse_leave_cb),
                     (gpointer)sensors_applet);

    g_signal_connect(G_OBJECT(sensors_applet->applet), "enter_notify_event",
                     G_CALLBACK(mouse_enter_cb),
                     (gpointer)sensors_applet);

    sensors_applet_update_active_sensors(sensors_applet);
    sensors_applet_pack_display(sensors_applet);

    sensors_applet->timeout_id = g_timeout_add_seconds(g_settings_get_int(sensors_applet->settings, TIMEOUT) / 1000,
                                                           (GSourceFunc)sensors_applet_update_active_sensors,
                                                           sensors_applet);
    gtk_widget_show_all(GTK_WIDGET(sensors_applet->applet));
}

