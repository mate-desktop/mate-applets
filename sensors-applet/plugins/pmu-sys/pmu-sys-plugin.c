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

#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "pmu-sys-plugin.h"

const gchar *plugin_name = "pmu-sys";

#define PMU_SYS_BASE_DIR "/sys/devices/temperatures/"
#define PMU_WT_SYS_BASE_DIR "/sys/devices/temperature/"

#define SENSOR1 "sensor1"
#define CPU "cpu"
#define SENSOR2 "sensor2"
#define GPU "gpu"
#define CASE "case"
#define TEMPERATURE "_temperature"
#define FAN_SPEED "_fan_speed"

enum {
    PMU_SYS_DEVICE_FILE_OPEN_ERROR,
    PMU_SYS_DEVICE_FILE_READ_ERROR
};

static void pmu_sys_plugin_add_sensor(GList **sensors, const gchar *path) {
    gchar *filename;
    gchar *label = NULL;
    gboolean enable;
    SensorType sensor_type;
    IconType icon_type = GENERIC_ICON;

    filename = g_path_get_basename(path);

    if (g_ascii_strcasecmp(filename, SENSOR1 TEMPERATURE) == 0 || g_ascii_strcasecmp(filename, CPU TEMPERATURE) == 0) {
        label = g_strdup(_("CPU"));
        sensor_type = TEMP_SENSOR;
        enable = TRUE;
        icon_type = CPU_ICON;
    } else if (g_ascii_strcasecmp(filename, SENSOR2 TEMPERATURE) == 0 || g_ascii_strcasecmp(filename, GPU TEMPERATURE) == 0) {
        label = g_strdup(_("GPU"));
        sensor_type = TEMP_SENSOR;
        enable = TRUE;
        icon_type = GPU_ICON;
    } else if (g_ascii_strcasecmp(filename, CASE TEMPERATURE) == 0) {
        label = g_strdup(_("CASE"));
        sensor_type = TEMP_SENSOR;
        enable = TRUE;
        icon_type = CASE_ICON;
    } else if (g_ascii_strcasecmp(filename, SENSOR1 FAN_SPEED) == 0|| g_ascii_strcasecmp(filename, CPU FAN_SPEED) == 0) {
        label = g_strdup(_("FAN"));
        sensor_type = FAN_SENSOR;
        enable = TRUE;
        icon_type = FAN_ICON;
    } else {
        /* disable all other sensors */
        enable = FALSE;
    }

    /* only add these 3 sensors */
    if (enable) {
        sensors_applet_plugin_add_sensor(sensors,
                                         path,
                                         filename,
                                         label,
                                         sensor_type,
                                         enable,
                                         icon_type,
                                         DEFAULT_GRAPH_COLOR);

    }

    g_free(filename);
    g_free(label);
}

static void pmu_sys_plugin_test_sensor(GList **sensors, const gchar *path) {
    gchar *filename;
    filename = g_path_get_basename(path);
    if (g_ascii_strcasecmp(filename, SENSOR1 TEMPERATURE) == 0 ||
        g_ascii_strcasecmp(filename, SENSOR2 TEMPERATURE) == 0 ||
        g_ascii_strcasecmp(filename, SENSOR1 FAN_SPEED) == 0 ||
        g_ascii_strcasecmp(filename, CPU TEMPERATURE) == 0 ||
        g_ascii_strcasecmp(filename, GPU TEMPERATURE) == 0 ||
        g_ascii_strcasecmp(filename, CASE TEMPERATURE) == 0 ||
        g_ascii_strcasecmp(filename, CPU FAN_SPEED) == 0) {

        pmu_sys_plugin_add_sensor(sensors, path);
    }

    g_free(filename);
}

/* to be called to setup for sys sensors */
static
GList *pmu_sys_plugin_init(void) {
    GList *sensors = NULL;
    /* call function to recursively look for sensors
       starting at the defined base directory */
    sensors_applet_plugin_find_sensors(&sensors, PMU_SYS_BASE_DIR, pmu_sys_plugin_test_sensor);
    sensors_applet_plugin_find_sensors(&sensors, PMU_WT_SYS_BASE_DIR, pmu_sys_plugin_test_sensor);
    return sensors;
}

/* returns the value of the sensor_list at the given iter, or if an
   error occurs, instatiates error with an error message */
static
gdouble pmu_sys_plugin_get_sensor_value(const gchar *path,
                                        const gchar *id,
                                        SensorType type,
                                        GError **error) {

    /* to open and access the value of each sensor */
    FILE *fp;
    gfloat sensor_value, dummy;

    if (NULL == (fp = fopen(path, "r"))) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, PMU_SYS_DEVICE_FILE_OPEN_ERROR, "Error opening sensor device file %s", path);
        return -1.0;
    }
    switch(type) {
        case FAN_SENSOR:
            if (fscanf(fp, "%f (%f rpm)", &dummy, &sensor_value) != 2) {
                g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, PMU_SYS_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                fclose(fp);
                return -1.0;
            }
            break;

        case TEMP_SENSOR:
            if (fscanf(fp, "%f", &sensor_value) != 1) {
                g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, PMU_SYS_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                fclose(fp);
                return -1.0;
            }
            break;

        default:
            /* should only have added temp or fan sensors */
            g_error("Unknown sensor type passed as parameter to pmu-sys sensor interface, cannot get value for this sensor");
            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, PMU_SYS_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
            fclose(fp);
            return -1.0;

    }
    fclose(fp);

    return (gdouble)sensor_value;
}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return pmu_sys_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return pmu_sys_plugin_get_sensor_value(path, id, type, error);
}
