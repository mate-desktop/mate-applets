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

#ifndef SENSORS_APPLET_PLUGINS_H
#define SENSORS_APPLET_PLUGINS_H

#include "sensors-applet.h"

typedef const gchar *(*SensorsAppletPluginName)(void);
typedef GList *(*SensorsAppletPluginInit)(void);
typedef gdouble (*SensorsAppletPluginGetSensorValue)(const gchar *path,
                                                     const gchar *id,
                                                     SensorType type,
                                                     GError **error);

void sensors_applet_plugins_load_all(SensorsApplet *sensors_applet);
void sensors_applet_plugins_unload_all(SensorsApplet *sensors_applet);
SensorsAppletPluginGetSensorValue sensors_applet_plugins_get_sensor_value_func(SensorsApplet *sensors_applet,
                                                                               const gchar *plugin);

#endif // SENSORS_APPLET_PLUGINS_H
