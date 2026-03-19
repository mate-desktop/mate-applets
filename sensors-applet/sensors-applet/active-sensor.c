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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include "active-sensor.h"
#include "sensors-applet-plugins.h"
#include "sensors-applet-settings.h"

typedef enum {
    VERY_LOW_SENSOR_VALUE = 0,
    LOW_SENSOR_VALUE,
    NORMAL_SENSOR_VALUE,
    HIGH_SENSOR_VALUE,
    VERY_HIGH_SENSOR_VALUE
} SensorValueRange;

/* Cast a given value to a valid SensorValueRange */
#define SENSOR_VALUE_RANGE(x) ((SensorValueRange)(CLAMP(x, VERY_LOW_SENSOR_VALUE, VERY_HIGH_SENSOR_VALUE)))

#define CAIRO_GRAPH_COLOR_GRADIENT 0.4

static const gchar * const temp_overlay_icons[] = {
    PIXMAPS_DIR "very-low-temp-icon.png",
    PIXMAPS_DIR "low-temp-icon.png",
    PIXMAPS_DIR "normal-temp-icon.png",
    PIXMAPS_DIR "high-temp-icon.png",
    PIXMAPS_DIR "very-high-temp-icon.png"
};

static gdouble sensor_value_range_normalised(gdouble value,
                                          gdouble low_value,
                                          gdouble high_value) {

    return ((value - low_value)/(high_value - low_value));
}

static SensorValueRange sensor_value_range(gdouble sensor_value,
                                            gdouble low_value,
                                            gdouble high_value) {

    gdouble range;
    range = sensor_value_range_normalised(sensor_value, low_value, high_value)*(gdouble)(VERY_HIGH_SENSOR_VALUE);

    /* check if need to round up, otherwise let int conversion
     * round down for us and make sure it is a valid range
     * value */
    return SENSOR_VALUE_RANGE(((gint)range + ((range - ((gint)range)) >= 0.5)));
}

static gboolean active_sensor_execute_alarm(ActiveSensor *active_sensor,
                                            NotifType notif_type) {

    gboolean ret;
    GError *error = NULL;

    sensors_applet_notify_active_sensor(active_sensor, notif_type);
    g_debug("EXECUTING %s ALARM: %s",
            (notif_type == LOW_ALARM ?
             "LOW" : "HIGH"),
            active_sensor->alarm_command[notif_type]);
    ret = g_spawn_command_line_async (active_sensor->alarm_command[notif_type], &error);
    g_debug("Command executed in shell");

    if (error)
        g_error_free (error);

    return ret;
}

static gboolean active_sensor_execute_low_alarm(ActiveSensor *active_sensor) {
    return active_sensor_execute_alarm(active_sensor, LOW_ALARM);
}

static gboolean active_sensor_execute_high_alarm(ActiveSensor *active_sensor) {
    return active_sensor_execute_alarm(active_sensor, HIGH_ALARM);
}

/* needs to be able to be called by the config dialog when the alarm command changes */
void active_sensor_alarm_off(ActiveSensor *active_sensor,
                             NotifType notif_type) {

    g_assert(active_sensor);

    if (active_sensor->alarm_timeout_id[notif_type] != -1) {
        g_debug("Disabling %s alarm.", (notif_type == LOW_ALARM ? "LOW" : "HIGH"));
        if (!g_source_remove(active_sensor->alarm_timeout_id[notif_type])) {
            g_debug("Error removing alarm source");
        }
        g_free(active_sensor->alarm_command[notif_type]);
        active_sensor->alarm_timeout_id[notif_type] = -1;

    }
    sensors_applet_notify_end(active_sensor, notif_type);
}

static void active_sensor_all_alarms_off(ActiveSensor *active_sensor) {
    /* turn off any alarms */
    int i;
    for (i = 0; i < NUM_ALARMS; i++) {
        if (active_sensor->alarm_timeout_id[i] >= 0) {
            g_debug("-- turning off notif with type %d ---", i);
            active_sensor_alarm_off(active_sensor, i);
        }
    }
}

static void active_sensor_alarm_on(ActiveSensor *active_sensor,
                                   NotifType notif_type) {

    GtkTreeModel *model;
    GtkTreePath *tree_path;
    GtkTreeIter iter;

    g_assert(active_sensor);

    model = gtk_tree_row_reference_get_model(active_sensor->sensor_row);
    tree_path = gtk_tree_row_reference_get_path(active_sensor->sensor_row);

    if (gtk_tree_model_get_iter(model, &iter, tree_path)) {

        if (active_sensor->alarm_timeout_id[notif_type] == -1) {
            /* alarm is not currently on */
            gtk_tree_model_get(model,
                       &iter,
                       (notif_type == LOW_ALARM ?
                            LOW_ALARM_COMMAND_COLUMN :
                            HIGH_ALARM_COMMAND_COLUMN),
                       &(active_sensor->alarm_command[notif_type]),
                       ALARM_TIMEOUT_COLUMN, &(active_sensor->alarm_timeout),
                       -1);

            g_debug("Activating alarm to repeat every %d seconds", active_sensor->alarm_timeout);

            /* execute alarm once, then add to time to keep repeating it */
            active_sensor_execute_alarm(active_sensor, notif_type);
            int timeout = (active_sensor->alarm_timeout <= 0 ? G_MAXINT : active_sensor->alarm_timeout);
            switch (notif_type) {
                case LOW_ALARM:
                    active_sensor->alarm_timeout_id[notif_type] = g_timeout_add_seconds(timeout,
                                                                                        (GSourceFunc)active_sensor_execute_low_alarm,
                                                                                        active_sensor);
                    break;

                case HIGH_ALARM:
                    active_sensor->alarm_timeout_id[notif_type] = g_timeout_add_seconds(timeout,
                                                                                        (GSourceFunc)active_sensor_execute_high_alarm,
                                                                                        active_sensor);
                    break;

                default:
                    g_debug("Unknown notif type: %d", notif_type);
            }

        }
    }

    gtk_tree_path_free(tree_path);
}

/**
 * Compares two ActiveSensors and returns -1 if a comes before b in the tree,
 * 0 if refer to same row, 1 if b comes before a
 */
gint active_sensor_compare(ActiveSensor *a, ActiveSensor *b) {
    GtkTreePath *a_tree_path, *b_tree_path;
    gint ret_val;

    g_assert(a);
    g_assert(b);

    a_tree_path = gtk_tree_row_reference_get_path(a->sensor_row);
    b_tree_path = gtk_tree_row_reference_get_path(b->sensor_row);

    ret_val = gtk_tree_path_compare(a_tree_path, b_tree_path);

    gtk_tree_path_free(a_tree_path);
    gtk_tree_path_free(b_tree_path);

    return ret_val;
}

static void active_sensor_update_icon(ActiveSensor *active_sensor,
                                      GdkPixbuf *base_icon,
                                      SensorType sensor_type) {

    GdkPixbuf *new_icon;
    const gchar *overlay_icon_filename = NULL;
    SensorValueRange value_range;

    g_assert(active_sensor);

    /* select overlay icon depending on sensor value */
    value_range = sensor_value_range(active_sensor->sensor_values[0],
                                     active_sensor->sensor_low_value,
                                     active_sensor->sensor_high_value);

    if (sensor_type == TEMP_SENSOR) {
        overlay_icon_filename = temp_overlay_icons[value_range];
    }

    /* load base icon */
    new_icon = gdk_pixbuf_copy(base_icon);

    /* only load overlay if required */
    if (overlay_icon_filename) {
        GdkPixbuf *overlay_icon;

        overlay_icon = gdk_pixbuf_new_from_file_at_size(overlay_icon_filename,
                                DEFAULT_ICON_SIZE,
                                DEFAULT_ICON_SIZE,
                                NULL);

        if (overlay_icon) {
            gdk_pixbuf_composite(overlay_icon, new_icon,
                                 0, 0,
                                 DEFAULT_ICON_SIZE, DEFAULT_ICON_SIZE,
                                 0, 0,
                                 1.0, 1.0,
                                 GDK_INTERP_BILINEAR,
                                 255);

            g_object_unref(overlay_icon);
        }
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(active_sensor->icon),
                              new_icon);
    g_object_unref(new_icon);

}

static void active_sensor_update_graph(ActiveSensor *as, cairo_t *cr) {
    GtkAllocation allocation;
    gdouble line_height;
    gdouble width, height;
    gdouble x = 0.0, y = 0.0;
    cairo_pattern_t *pattern;
    gint i;

    gtk_widget_get_allocation (as->graph, &allocation);
    width = allocation.width;
    height = allocation.height;

    /* so we can set a clipping area, as well as fill the * back of the graph black */
    cairo_rectangle(cr,
                    0, 0,
                    width,
                    height);
    /* clip to rectangle and keep it as a path so can be
     * filled below */
    cairo_clip_preserve(cr);

    /* use black for bg color of graphs */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_fill(cr);

    /* determine height to scale line at for each value -
     * only do as many as will fit or the number of
     * samples that we have */
    for (i = 0; i < MIN(as->num_samples, width); i++) {
        /* need to remove one more to make it line up
         * properly  when drawing */
        x = width - i - 1;
        y = height;

        line_height = sensor_value_range_normalised(as->sensor_values[i],
                                                    as->sensor_low_value,
                                                    as->sensor_high_value) * height;

        if (line_height > 0) {
            cairo_move_to(cr, x, y);
            cairo_line_to(cr, x, y - line_height);
        }
    }

    /* make lines a gradient from slightly darker than
     * chosen color at bottom of graph, to slightly
     * lighter than chosen color at top of graph */
    pattern = cairo_pattern_create_linear(x, y, x, 0);
    cairo_pattern_add_color_stop_rgb(pattern,
                                     0,
                                     as->graph_color.red - CAIRO_GRAPH_COLOR_GRADIENT,
                                     as->graph_color.green - CAIRO_GRAPH_COLOR_GRADIENT,
                                     as->graph_color.blue - CAIRO_GRAPH_COLOR_GRADIENT);

    cairo_pattern_add_color_stop_rgb(pattern,
                                     height,
                                     as->graph_color.red + CAIRO_GRAPH_COLOR_GRADIENT,
                                     as->graph_color.green + CAIRO_GRAPH_COLOR_GRADIENT,
                                     as->graph_color.blue + CAIRO_GRAPH_COLOR_GRADIENT);

    cairo_set_source(cr, pattern);
    cairo_stroke(cr);
    cairo_pattern_destroy(pattern);
}

void active_sensor_destroy(ActiveSensor *active_sensor) {
    g_debug("-- destroying active sensor label...");
    gtk_widget_destroy(active_sensor->label);

    g_debug("-- destroying active sensor icon..");
    gtk_widget_destroy(active_sensor->icon);

    g_debug("-- destroying active sensor value...");
    gtk_widget_destroy(active_sensor->value);

    g_debug("-- destroying active sensor graph and frame...");
    gtk_widget_destroy(active_sensor->graph);
    gtk_widget_destroy(active_sensor->graph_frame);

    g_debug("-- destroying active sensor values...");
    g_free(active_sensor->sensor_values);

    active_sensor_all_alarms_off(active_sensor);

    g_free(active_sensor);
}

static
gboolean graph_draw_cb(GtkWidget *graph,
                       cairo_t *cr,
                       gpointer data) {

    ActiveSensor *as;

    as = (ActiveSensor *)data;

    active_sensor_update_graph(as, cr);
    /* propagate event onwards */

    return FALSE;
}

static void active_sensor_set_graph_dimensions(ActiveSensor *as,
                                               gint width,
                                               gint height) {

    gint num_samples, old_num_samples;
    gint graph_width, graph_height;

    /* dimensions are really for graph frame, so need to remove
     * extra width added by graph frame - make sure not less than
     * 1 - always need atleast 1 sample */
    graph_width = CLAMP(width - GRAPH_FRAME_EXTRA_WIDTH, 1, width - GRAPH_FRAME_EXTRA_WIDTH);
    graph_height = CLAMP(height - GRAPH_FRAME_EXTRA_WIDTH, 1 , height - GRAPH_FRAME_EXTRA_WIDTH);

    g_debug("setting graph dimensions to %d x %d", graph_width, graph_height);
    num_samples = graph_width;

    if (as->sensor_values) {
        gdouble *old_values;

        old_values = as->sensor_values;
        old_num_samples = as->num_samples;

        as->num_samples = num_samples;
        as->sensor_values = g_new0 (gdouble, as->num_samples);
        memcpy(as->sensor_values,
               old_values,
               MIN(old_num_samples, as->num_samples)*sizeof(gdouble));

        g_free(old_values);
    } else {
        as->sensor_values = g_new0 (gdouble, num_samples);
        as->num_samples = num_samples;
    }

    /* update graph frame size request */
    gtk_widget_set_size_request(as->graph,
                                graph_width,
                                graph_height);
}

void active_sensor_update_graph_dimensions(ActiveSensor *as,
                                     gint sizes[2]) {

    active_sensor_set_graph_dimensions(as, sizes[0], sizes[1]);
    gtk_widget_queue_draw (as->graph);
}

ActiveSensor *active_sensor_new(SensorsApplet *sensors_applet,
                                GtkTreeRowReference *sensor_row) {

    ActiveSensor *active_sensor;
    MatePanelAppletOrient orient;
    gint graph_size;
    gboolean horizontal;

    g_assert(sensors_applet);
    g_assert(sensor_row);

    g_debug("creating new active sensor");

    active_sensor = g_new0(ActiveSensor, 1);
    active_sensor->sensors_applet = sensors_applet;

    active_sensor->sensor_row = sensor_row;

#ifdef HAVE_LIBNOTIFY
    /* set a base time, creation -5 minutes */
    active_sensor->ierror_ts = time(NULL) - 300;
#endif

    int i;
    for (i = 0; i < NUM_ALARMS; i++) {
        active_sensor->alarm_timeout_id[i] = -1;
    }

    active_sensor->label = gtk_label_new("");
    active_sensor->value = gtk_label_new("");
    active_sensor->icon = gtk_image_new();

    active_sensor->graph = gtk_drawing_area_new();
    active_sensor->graph_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(active_sensor->graph_frame),
                              GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(active_sensor->graph_frame),
                      active_sensor->graph);
    gtk_widget_add_events(active_sensor->graph_frame,
                          GDK_ALL_EVENTS_MASK);

    /* need to set size according to orientation */
    orient = mate_panel_applet_get_orient(active_sensor->sensors_applet->applet);
    graph_size = g_settings_get_int(active_sensor->sensors_applet->settings, GRAPH_SIZE);

    horizontal = ((orient == MATE_PANEL_APPLET_ORIENT_UP) ||
                  (orient == MATE_PANEL_APPLET_ORIENT_DOWN));

    active_sensor_set_graph_dimensions(active_sensor,
                                       (horizontal ? graph_size : sensors_applet->size),
                                       (horizontal ? sensors_applet->size : graph_size));

    g_signal_connect(G_OBJECT(active_sensor->graph),
                     "draw",
                     G_CALLBACK(graph_draw_cb),
                     active_sensor);

    active_sensor->updated = FALSE;
    return active_sensor;
}

static void active_sensor_update_sensor_value(ActiveSensor *as,
                                              gdouble sensor_value) {

    /* only if have more than 1 sample stored */
    if (as->num_samples > 1) {
        memmove(&(as->sensor_values[1]),
                as->sensor_values,
                (as->num_samples - 1)*sizeof(gdouble));
    }

    as->sensor_values[0] = sensor_value;
}

void active_sensor_update(ActiveSensor *active_sensor,
                          SensorsApplet *sensors_applet) {

    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;

    /* instance data from the tree for this sensor */
    gchar *sensor_path = NULL;
    gchar *sensor_id = NULL;
    gchar *sensor_label = NULL;
    SensorType sensor_type;
    gchar *sensor_interface;
    gboolean sensor_enabled;
    gdouble sensor_low_value;
    gdouble sensor_high_value;
    gboolean sensor_alarm_enabled;
    gdouble sensor_multiplier;
    gdouble sensor_offset;
    gdouble sensor_value;
    GdkPixbuf *icon_pixbuf;
    gchar *graph_color;
    gint label_min_width;
    GtkRequisition  req;

    /* to build the list of labels as we go */
    gchar *value_text = NULL;

    TemperatureScale scale;
    DisplayMode display_mode;

    GError *error = NULL;

    gchar *tooltip = NULL;
    gchar *value_tooltip = NULL;

    /* hidden gsettings options */
    gint font_size = 0;
    gboolean hide_units = FALSE;
    gboolean hide_units_old = FALSE;

    g_assert(active_sensor);
    g_assert(active_sensor->sensor_row);
    g_assert(sensors_applet);

    model = gtk_tree_row_reference_get_model(active_sensor->sensor_row);
    path = gtk_tree_row_reference_get_path(active_sensor->sensor_row);

    /* if can successfully get iter can proceed */
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_path_free(path);
        gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors),
                   &iter,
                   PATH_COLUMN, &sensor_path,
                   ID_COLUMN, &sensor_id,
                   LABEL_COLUMN, &sensor_label,
                   INTERFACE_COLUMN, &sensor_interface,
                   SENSOR_TYPE_COLUMN, &sensor_type,
                   ENABLE_COLUMN, &sensor_enabled,
                   LOW_VALUE_COLUMN, &sensor_low_value,
                   HIGH_VALUE_COLUMN, &sensor_high_value,
                   ALARM_ENABLE_COLUMN, &sensor_alarm_enabled,
                   MULTIPLIER_COLUMN, &sensor_multiplier,
                   OFFSET_COLUMN, &sensor_offset,
                   ICON_PIXBUF_COLUMN, &icon_pixbuf,
                   GRAPH_COLOR_COLUMN, &graph_color,
                   -1);

        SensorsAppletPluginGetSensorValue get_sensor_value;
        /* only call function if is in hash table for plugin */
        if ((get_sensor_value = sensors_applet_plugins_get_sensor_value_func(sensors_applet, sensor_interface)) != NULL) {
            sensor_value = get_sensor_value(sensor_path,
                                            sensor_id,
                                            sensor_type,
                                            &error);

            if (error) {
                g_debug("Error updating active sensor: %s", error->message);
                sensors_applet_notify_active_sensor(active_sensor,
                                                    SENSOR_INTERFACE_ERROR);

                /* hard code text as ERROR */
                value_text = g_strdup(_("ERROR"));
                value_tooltip = g_strdup_printf("- %s", error->message);
                g_error_free(error);
                error = NULL;

                /* set sensor value to an error code -
                 * note this is not unique */
                sensor_value = -1;
            } else {
                /* use hidden gsettings key for hide_units */
                hide_units_old = hide_units;
                hide_units = g_settings_get_boolean(sensors_applet->settings, HIDE_UNITS);

                /* scale value and set text using this value */
                switch (sensor_type) {
                    case TEMP_SENSOR:
                        scale = (TemperatureScale) g_settings_get_int(sensors_applet->settings, TEMPERATURE_SCALE);
                        /* scale value */
                        sensor_value = sensors_applet_convert_temperature(sensor_value, CELSIUS, scale);

                        sensor_value = (sensor_value * sensor_multiplier) + sensor_offset;
                        switch (scale) {
                            case FAHRENHEIT:
                                value_text = g_strdup_printf("%2.0f %s", sensor_value, (hide_units ? "" : UNITS_FAHRENHEIT));
                                /* tooltip should
                                 * always display
                                 * units */
                                value_tooltip = g_strdup_printf("%2.0f %s", sensor_value, UNITS_FAHRENHEIT);

                                break;
                            case CELSIUS:
                                value_text = g_strdup_printf("%2.0f %s", sensor_value, (hide_units ? "" : UNITS_CELSIUS));
                                value_tooltip = g_strdup_printf("%2.0f %s", sensor_value, UNITS_CELSIUS);
                                break;
                            case KELVIN:
                                value_text = g_strdup_printf("%2.0f", sensor_value);
                                value_tooltip = g_strdup(value_text);
                                break;
                        }
                        break;

                    case FAN_SENSOR:
                        sensor_value = (sensor_value * sensor_multiplier) + sensor_offset;
                        value_text = g_strdup_printf("%4.0f %s", sensor_value, (hide_units ? "" : UNITS_RPM));
                        value_tooltip = g_strdup_printf("%4.0f %s", sensor_value, UNITS_RPM);
                        break;

                    case VOLTAGE_SENSOR:
                        sensor_value = (sensor_value * sensor_multiplier) + sensor_offset;
                        value_text = g_strdup_printf("%4.2f %s", sensor_value, (hide_units ? "" : UNITS_VOLTAGE));
                        value_tooltip = g_strdup_printf("%4.2f %s", sensor_value, UNITS_VOLTAGE);
                        break;

                    case CURRENT_SENSOR:
                        sensor_value = (sensor_value * sensor_multiplier) + sensor_offset;
                        value_text = g_strdup_printf("%4.2f %s", sensor_value, (hide_units ? "" : UNITS_CURRENT));
                        value_tooltip = g_strdup_printf("%4.2f %s", sensor_value, UNITS_CURRENT);
                        break;

                } /* end switch(sensor_type) */
            } /* end else on error */

            /* setup for tooltips */
            if (sensors_applet->show_tooltip)
                    tooltip = g_strdup_printf("%s %s", sensor_label, value_tooltip);
            g_free(value_tooltip);

            /* only do icons and labels / graphs if needed */
            display_mode = g_settings_get_int (sensors_applet->settings, DISPLAY_MODE);

            /* most users wont have a font size set */
            font_size = g_settings_get_int (sensors_applet->settings, FONT_SIZE);

            /* do icon if needed */
            if (display_mode == DISPLAY_ICON ||
                display_mode == DISPLAY_ICON_WITH_VALUE) {

                /* update icon if icon range has changed if no
                 * update has been done before */
                if ((sensor_value_range(sensor_value, sensor_low_value, sensor_high_value) != sensor_value_range(active_sensor->sensor_values[0], active_sensor->sensor_low_value, active_sensor->sensor_high_value)) ||
                    !(active_sensor->updated)) {

                    active_sensor_update_sensor_value(active_sensor, sensor_value);
                    active_sensor->sensor_low_value = sensor_low_value;
                    active_sensor->sensor_high_value = sensor_high_value;
                    active_sensor_update_icon(active_sensor, icon_pixbuf, sensor_type);
                }

                if (tooltip) {
                    gtk_widget_set_tooltip_text(active_sensor->icon, tooltip);
                }
            }

            active_sensor_update_sensor_value(active_sensor, sensor_value);
            active_sensor->sensor_low_value = sensor_low_value;
            active_sensor->sensor_high_value = sensor_high_value;

            /* do graph if needed */
            if (display_mode == DISPLAY_GRAPH) {
                /* update graph color in case has changed */
                gdk_rgba_parse(&(active_sensor->graph_color), graph_color);

                gtk_widget_queue_draw (active_sensor->graph);
                if (tooltip) {
                    gtk_widget_set_tooltip_text(active_sensor->graph, tooltip);
                }
            }

            gchar *old_value_text = value_text;

            if (sensor_alarm_enabled) {
                if (sensor_value >= sensor_high_value || sensor_value <= sensor_low_value) {

                    /* make value text red and
                     * activate alarm */
                    if (display_mode == DISPLAY_LABEL_WITH_VALUE ||
                        display_mode == DISPLAY_ICON_WITH_VALUE ||
                        display_mode == DISPLAY_VALUE) {

                        value_text = g_markup_printf_escaped("<span foreground=\"#FF0000\">%s</span>", old_value_text);
                        g_free(old_value_text);
                    }

                    /* could have both coditions at once */
                    if (sensor_value >= sensor_high_value) {
                        active_sensor_alarm_on(active_sensor, HIGH_ALARM);
                    }

                    if (sensor_value <= sensor_low_value) {
                        active_sensor_alarm_on(active_sensor, LOW_ALARM);
                    }

                } else {
                    /* make sure alarms are off */
                    active_sensor_all_alarms_off(active_sensor);
                }
            } else {
                /* else for if alarm enabled */
                /* make sure all alarms are off */
                active_sensor_all_alarms_off(active_sensor);
            }

            /* do value label */
            if (display_mode == DISPLAY_LABEL_WITH_VALUE ||
                display_mode == DISPLAY_ICON_WITH_VALUE ||
                display_mode == DISPLAY_VALUE) {

                if (font_size) {
                    old_value_text = value_text;

                    value_text = g_strdup_printf("<span font_desc=\"%d\">%s</span>", font_size, old_value_text);
                    g_free(old_value_text);
                }

                /*Keep the label as large as previous size unless hide_units has changed */
                gtk_widget_get_preferred_size (GTK_WIDGET (active_sensor->value), &req, NULL);
                if (!(hide_units_old == hide_units)) {
                    label_min_width = 0;
                } else {
                    label_min_width = req.width;
                }

                gtk_label_set_markup(GTK_LABEL(active_sensor->value), value_text);

                gtk_widget_get_preferred_size (GTK_WIDGET (active_sensor->value), &req, NULL);
                gtk_widget_set_size_request (GTK_WIDGET (active_sensor->value), label_min_width, req.height);

                if (tooltip) {
                    gtk_widget_set_tooltip_text(active_sensor->value, tooltip);
                }
            }
            /* finished with value text */
            g_free(value_text);

            /* do label label */
            if (display_mode == DISPLAY_LABEL_WITH_VALUE) {
                if (font_size) {
                    old_value_text = sensor_label;
                    sensor_label = g_strdup_printf("<span font_desc=\"%d\">%s</span>", font_size, old_value_text);
                    g_free(old_value_text);
                }

                gtk_label_set_markup(GTK_LABEL(active_sensor->label), sensor_label);
                if (tooltip) {
                        gtk_widget_set_tooltip_text(active_sensor->label, tooltip);
                }
            }

            g_free(tooltip);
        } else {
            g_debug("no get_sensor_value function yet installed for interface %s.", sensor_interface);
        }

        g_free(sensor_path);
        g_free(sensor_id);
        g_free(sensor_label);
        g_free(sensor_interface);
        g_free(graph_color);
        g_object_unref(icon_pixbuf);

    } else {
        g_debug("Error getting iter when updating sensor...");
    }

    active_sensor->updated = TRUE;

}

/* to be called when the icon within the GtkRowReference that this
 * sensor references is changed - updates icon based upon value in the
 * ActiveSensor */
void active_sensor_icon_changed(ActiveSensor *active_sensor, SensorsApplet *sensors_applet) {

    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;

    SensorType sensor_type;
    GdkPixbuf *icon_pixbuf;

    g_assert(active_sensor);
    g_assert(sensors_applet);

    model = gtk_tree_row_reference_get_model(active_sensor->sensor_row);
    path = gtk_tree_row_reference_get_path(active_sensor->sensor_row);

    /* if can successfully get iter can proceed */
    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors),
                                           &iter,
                                           SENSOR_TYPE_COLUMN, &sensor_type,
                                           ICON_PIXBUF_COLUMN, &icon_pixbuf,
                                           -1);

        active_sensor_update_icon(active_sensor, icon_pixbuf, sensor_type);
        g_object_unref(icon_pixbuf);
    }
    gtk_tree_path_free(path);
}
