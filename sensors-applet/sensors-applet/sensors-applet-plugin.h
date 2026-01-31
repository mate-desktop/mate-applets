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

#ifndef SENSORS_APPLET_PLUGIN_H
#define SENSORS_APPLET_PLUGIN_H

#include <sensors-applet/sensors-applet-sensor.h>

GQuark sensors_applet_plugin_error_quark(void);

/* for error handling */
#define SENSORS_APPLET_PLUGIN_ERROR (sensors_applet_plugin_error_quark())

/**
 * Initialises the plugin, and returns the list of SensorsAppletSensorInfo
 * structs to create the sensors for this plugin from
 */
GList *sensors_applet_plugin_init(void);
gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                               const gchar *id,
                                               SensorType type,
                                               GError **error);

const gchar *sensors_applet_plugin_name(void);

typedef void SensorsAppletPluginTestSensorFunc(GList **sensors,
                                               const gchar *path);
void sensors_applet_plugin_find_sensors(GList **sensors,
                                        const gchar *path,
                                        SensorsAppletPluginTestSensorFunc);

void sensors_applet_plugin_default_sensor_limits(SensorType type,
                                                 gdouble *low_value,
                                                 gdouble *high_value);

void sensors_applet_plugin_add_sensor(GList **sensors,
                                      const gchar *path,
                                      const gchar *id,
                                      const gchar *label,
                                      SensorType type,
                                      gboolean enable,
                                      IconType icon,
                                      const gchar *graph_color);

void sensors_applet_plugin_add_sensor_with_limits(GList **sensors,
                                                  const gchar *path,
                                                  const gchar *id,
                                                  const gchar *label,
                                                  SensorType type,
                                                  gboolean enable,
                                                  gdouble low_value,
                                                  gdouble high_value,
                                                  IconType icon,
                                                  const gchar *graph_color);

#endif // SENSORS_APPLET_PLUGIN_H
