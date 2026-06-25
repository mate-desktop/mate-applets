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

#include <sensors-applet/sensors-applet-plugin.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

extern const gchar *plugin_name;

/* recursive function to find sensors in a given path */
void sensors_applet_plugin_find_sensors(GList **sensors,
                                        const gchar *path,
                                        SensorsAppletPluginTestSensorFunc test_sensor) {
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        /* also test can actually open file for reading */
        if (access(path, R_OK) == 0) {
            test_sensor(sensors, path);
        }
    }

    /* if is a directory (but not a symlinked dir as this
       will lead us in circular loops) descend into it and look
       for a sensor dir
    */
    if (g_file_test(path, G_FILE_TEST_IS_DIR) && !g_file_test(path, G_FILE_TEST_IS_SYMLINK)) {
        GDir *dir;

        if ((dir = g_dir_open(path, 0, NULL)) != NULL) {
            const gchar *new_file;

            while(NULL != (new_file = g_dir_read_name(dir))) {
                gchar *new_path;

                new_path = g_build_filename(path, new_file, NULL);
                sensors_applet_plugin_find_sensors(sensors, new_path, test_sensor);
                g_free(new_path);
            }
            g_dir_close(dir);
        }
    }
}

/* for error handling */
GQuark sensors_applet_plugin_error_quark(void) {
    static GQuark quark = 0;

    if (quark == 0) {
        gchar *string;

        string = g_strdup_printf("%s-plugin-error", plugin_name);
        quark = g_quark_from_string(string);
        g_free(string);
    }

    return quark;
}

void sensors_applet_plugin_default_sensor_limits(SensorType type,
                                                 gdouble *low_value,
                                                 gdouble *high_value) {

    switch (type) {
    case TEMP_SENSOR:
        *low_value = 20.0;
        *high_value = 60.0;
        break;
    case FAN_SENSOR:
        *low_value = 600.0;
        *high_value = 3000.0;
        break;
    default:
        *low_value = 0.0;
        *high_value = 0.0;
    }
}

void sensors_applet_plugin_add_sensor(GList **sensors,
                                      const gchar *path,
                                      const gchar *id,
                                      const gchar *label,
                                      SensorType type,
                                      gboolean enable,
                                      IconType icon,
                                      const gchar *graph_color) {

    gdouble low_value;
    gdouble high_value;
    sensors_applet_plugin_default_sensor_limits(type, &low_value, &high_value);

    sensors_applet_plugin_add_sensor_with_limits(sensors,
                                              path,
                                              id,
                                              label,
                                              type,
                                              enable,
                                              low_value,
                                              high_value,
                                              icon,
                                              graph_color);

}

void sensors_applet_plugin_add_sensor_with_limits(GList **sensors,
                                                  const gchar *path,
                                                  const gchar *id,
                                                  const gchar *label,
                                                  SensorType type,
                                                  gboolean enable,
                                                  gdouble low_value,
                                                  gdouble high_value,
                                                  IconType icon,
                                                  const gchar *graph_color) {

    SensorsAppletSensorInfo *info;

    info = g_new0 (SensorsAppletSensorInfo, 1);

    info->path = g_strdup(path);
    info->id = g_strdup(id);
    info->label = g_strdup(label);
    info->type = type;
    info->enable = enable;
    info->low_value = low_value;
    info->high_value = high_value;
    info->multiplier = 1.0;
    info->offset = 0.0;
    info->icon = icon;
    info->graph_color = g_strdup(graph_color);

    *sensors = g_list_append(*sensors, info);
}

