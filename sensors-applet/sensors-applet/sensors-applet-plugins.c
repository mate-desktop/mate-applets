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

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif /* HAVE_DLFCN_H */

#include "sensors-applet-plugins.h"
#include "sensors-applet-sensor.h"

#define SENSORS_APPLET_USER_PLUGIN_DIR ".mate2/sensors-applet/plugins"

static void load_all_plugins(SensorsApplet *sensors_applet, const gchar *path) {

    if (g_file_test(path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
        GDir *dir;
        if ((dir = g_dir_open(path, 0, NULL)) != NULL) {
            const gchar *file;
            while ((file = g_dir_read_name(dir)) != NULL) {
                /* try and open plugin */
                gchar *plugin_file;
                void *handle;
                SensorsAppletPluginInit init_fn;

                plugin_file = g_strdup_printf("%s/%s", path, file);
                g_debug("found %s in plugin directory", plugin_file);
                if ((handle = dlopen(plugin_file, RTLD_NOW)) != NULL) {
                    SensorsAppletPluginName name_fn;
                    SensorsAppletPluginGetSensorValue get_value_fn;

                    if ((name_fn = dlsym(handle, "sensors_applet_plugin_name")) != NULL &&
                        (init_fn = dlsym(handle, "sensors_applet_plugin_init")) != NULL &&
                        (get_value_fn = dlsym(handle, "sensors_applet_plugin_get_sensor_value")) != NULL) {

                        GList *sensors;
                        g_debug("calling init function for plugin file %s", plugin_file);

                        if ((sensors = init_fn()) != NULL) {
                            GList *sensor;

                            g_debug("registering plugin %s", name_fn());
                            g_hash_table_insert(sensors_applet->plugins, g_strdup(name_fn()), get_value_fn);

                            for (sensor = g_list_first(sensors); sensor != NULL; sensor = g_list_next(sensor)) {

                                SensorsAppletSensorInfo *sensor_info = (SensorsAppletSensorInfo *)sensor->data;
                                sensors_applet_add_sensor(sensors_applet,
                                                          sensor_info->path,
                                                          sensor_info->id,
                                                          sensor_info->label,
                                                          name_fn(),
                                                          sensor_info->type,
                                                          sensor_info->enable,
                                                          sensor_info->low_value,
                                                          sensor_info->high_value,
                                                          FALSE, /* ALARM OFF */
                                                          "", /* no alarm commands */
                                                          "", /* no alarm commands */
                                                          0, /* alarm_timeout */
                                                          sensor_info->multiplier,
                                                          sensor_info->offset,
                                                          sensor_info->icon,
                                                          sensor_info->graph_color);

                                // sensors_applet_add_sensor() doesn't free strings, so free them here
                                g_free(sensor_info->path);
                                g_free(sensor_info->id);
                                g_free(sensor_info->label);
                                g_free(sensor_info->graph_color);
                                g_free(sensor_info);
                            }
                            g_list_free(sensors);
                        } else {
                            g_debug("plugin could not find any sensors");
                            if (g_hash_table_lookup(sensors_applet->required_plugins, name_fn())) {
                                g_debug("plugin is required - registering even though no sensors detected");
                                g_debug("registering plugin %s", name_fn());
                                g_hash_table_insert(sensors_applet->plugins, g_strdup(name_fn()), get_value_fn);
                            } else {
                                g_debug("unloading plugin");
                            }
                        }

                    } else {
                        g_debug("plugin file %s does not contain the required interface", plugin_file);
                        if (dlclose(handle) != 0) {
                            g_debug("error closing plugin file %s", plugin_file);
                        }
                    }
                } else {
                    g_debug("Could not dlopen: %s: %s", plugin_file, dlerror());
                }
                g_free(plugin_file);
            }
            g_dir_close(dir);
        } else {
            g_debug("error opening plugin dir %s", path);
        }
    } else {
        g_debug("path %s is not a valid directory", path);
    }
}

void sensors_applet_plugins_load_all(SensorsApplet *sensors_applet) {
    const gchar *home;

    if ((home = g_get_home_dir()) != NULL) {
        gchar *path;
        path = g_build_filename(home, SENSORS_APPLET_USER_PLUGIN_DIR, NULL);
        load_all_plugins(sensors_applet, path);
        g_free(path);
    } else {
        g_warning("could not get home dir of user");
    }

    load_all_plugins(sensors_applet, SENSORS_APPLET_PLUGIN_DIR);
}

SensorsAppletPluginGetSensorValue sensors_applet_plugins_get_sensor_value_func(SensorsApplet *sensors_applet, const gchar *plugin) {
    return g_hash_table_lookup(sensors_applet->plugins, plugin);
}

