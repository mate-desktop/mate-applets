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

#ifndef SENSORS_APPLET_SENSOR_H
#define SENSORS_APPLET_SENSOR_H

#include <glib.h>

/* forward declare for plugins to use */
typedef struct _SensorsApplet SensorsApplet;

#define DEFAULT_GRAPH_COLOR "#ff0000"

/* device icons */
typedef enum {
    CPU_ICON = 0,
    HDD_ICON,
    BATTERY_ICON,
    MEMORY_ICON,
    GPU_ICON,
    GENERIC_ICON,
    FAN_ICON,
    CASE_ICON,
    NUM_ICONS,
} IconType;

typedef enum {
    CURRENT_SENSOR = 0,
    FAN_SENSOR,
    TEMP_SENSOR,
    VOLTAGE_SENSOR
} SensorType;

typedef struct _SensorsAppletSensorInfo {
    gchar *path; /* must be dynamically allocated */
    gchar *id; /* must be dynamically allocated */
    gchar *label; /* must be dynamically allocated */
    SensorType type;
    gboolean enable;
    gdouble low_value;
    gdouble high_value;
    gdouble multiplier;
    gdouble offset;
    IconType icon;
    gchar *graph_color; /* must be dynamically allocated */
} SensorsAppletSensorInfo;

#endif // SENSORS_APPLET_SENSOR_H
