/*
 * Copyright (C) 2005-2009 Alex Murray <murray.alex@gmail.com>
 *               2013 Stefano Karapetsas <stefano@karapetsas.com>
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

#ifndef SENSORS_APPLET_SETTINGS_H
#define SENSORS_APPLET_SETTINGS_H

#include "sensors-applet.h"
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

#define FONT_SIZE             "font-size"
#define HIDE_UNITS            "hide-units"
#define DISPLAY_MODE          "display-mode"
#define LAYOUT_MODE           "layout-mode"
#define TEMPERATURE_SCALE     "temperature-scale"
#define DISPLAY_NOTIFICATIONS "display-notifications"
#define TIMEOUT               "timeout-delay"
#define GRAPH_SIZE            "graph-size"
#define SENSORS_LIST          "sensors-list"

#define PATH                  "path"
#define ID                    "id"
#define INTERFACE             "interface"
#define LABEL                 "label"
#define ENABLED               "enabled"
#define LOW_VALUE             "low-value"
#define HIGH_VALUE            "high-value"
#define ALARM_ENABLED         "alarm-enabled"
#define LOW_ALARM_COMMAND     "low-alarm-command"
#define HIGH_ALARM_COMMAND    "high-alarm-command"
#define ALARM_TIMEOUT         "alarm-timeout"
#define SENSOR_TYPE           "sensor-type"
#define MULTIPLIER            "multiplier"
#define OFFSET                "offset"
#define ICON_TYPE             "icon-type"
#define GRAPH_COLOR           "graph-color"

gchar* sensors_applet_settings_get_unique_id (const gchar *interface, const gchar *id, const gchar *path);
gboolean sensors_applet_settings_load_sensors (SensorsApplet *sensors_applet);
gboolean sensors_applet_settings_sort_sensors (SensorsApplet *sensors_applet);
gboolean sensors_applet_settings_save_sensors (SensorsApplet *sensors_applet);

#endif /* SENSORS_APPLET_SETTINGS_H*/
