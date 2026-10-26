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

#ifndef ACTIVE_SENSOR_H
#define ACTIVE_SENSOR_H

#ifdef HAVE_LIBNOTIFY
#include <time.h>
#include <libnotify/notify.h>
#include "active-sensor-libnotify.h"
#endif

#include "sensors-applet.h"

struct _ActiveSensor {
    SensorsApplet *sensors_applet;

    /* widgets to render to display the sensor */
    GtkWidget *label;
    GtkWidget *icon;
    GtkWidget *value;
    GtkWidget *graph;
    GtkWidget *graph_frame;

    GdkRGBA graph_color;

    GtkTreeRowReference *sensor_row;

#ifdef HAVE_LIBNOTIFY
    NotifyNotification *notification[NUM_NOTIFS];
    /* error timestamp - save the time of the last SENSOR_INTERFACE_ERROR */
    time_t ierror_ts;
#endif

    gboolean updated;

    /* alarm related stuff */
    gint alarm_timeout_id[NUM_ALARMS];
    gchar *alarm_command[NUM_ALARMS];
    gint alarm_timeout;

    /* buffer of sensor values */
    gdouble *sensor_values;

    /* length of sensor_values buffer */
    gint num_samples;

    gdouble sensor_low_value;
    gdouble sensor_high_value;
};

ActiveSensor *active_sensor_new(SensorsApplet *sensors_applet, GtkTreeRowReference *sensor_row);
void active_sensor_destroy(ActiveSensor *active_sensor);
gint active_sensor_compare(ActiveSensor *a, ActiveSensor *b);
void active_sensor_update(ActiveSensor *sensor, SensorsApplet *sensors_applet);
void active_sensor_icon_changed(ActiveSensor *sensor, SensorsApplet *sensors_applet);
void active_sensor_update_graph_dimensions(ActiveSensor *as, gint dimensions[2]);
void active_sensor_alarm_off(ActiveSensor *active_sensor, NotifType notif_type);

#endif /* ACTIVE_SENSOR_H */
