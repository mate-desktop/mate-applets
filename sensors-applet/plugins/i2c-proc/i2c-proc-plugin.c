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
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "i2c-proc-plugin.h"

const gchar *plugin_name = "i2c-proc";

#define I2C_PROC_BASE_DIR "/proc/sys/dev/sensors"

enum {
    I2C_PROC_DEVICE_FILE_OPEN_ERROR,
    I2C_PROC_DEVICE_FILE_READ_ERROR
};

static void i2c_proc_plugin_add_sensor(GList **sensors, const gchar *path) {
    gchar *filename;
    gchar *label;
    gboolean enable;
    SensorType sensor_type;
    IconType icon_type = GENERIC_ICON;

    filename = g_path_get_basename(path);

    /* setup temp2 as CPU sensor and enable it */
    if (g_ascii_strcasecmp(filename, "temp2") == 0) {
        sensor_type = TEMP_SENSOR;
        label = g_strdup(_("CPU"));
        enable = TRUE;
        icon_type = CPU_ICON;
    } else {
        label = g_strdup(filename);

        switch(filename[0]) {
        case 'c':     /* currents are "curr?" */
            sensor_type = CURRENT_SENSOR;
            break;
        case 'f':     /* fans are "fan?" */
            sensor_type = FAN_SENSOR;
            icon_type = FAN_ICON;
            break;
        case 'i':     /* voltages are "in?" */
            sensor_type = VOLTAGE_SENSOR;
            break;
        case 't':     /* temps are "temp?" */
            sensor_type = TEMP_SENSOR;
            break;
        case 'v':     /* vids are just vid */
            sensor_type = VOLTAGE_SENSOR;
            break;
        default:
            /* SHOULDN'T BE ABLE TO GET HERE!! */
            g_debug("error in i2c-proc sensor detection code - unhandled sensor filename %s", filename);
            g_free(filename);
            g_free(label);
            return;
        }
        /* disable all other sensors */
        enable = FALSE;
    }

    sensors_applet_plugin_add_sensor(sensors,
                                         path,
                                         filename,
                                         label,
                                         sensor_type,
                                         enable,
                                         icon_type,
                                         DEFAULT_GRAPH_COLOR);

    g_free(filename);
    g_free(label);
}

static void i2c_proc_plugin_test_sensor(GList **sensors, const gchar *path) {
    gchar *filename;

    filename = g_path_get_basename(path);
    /* see if filename starts with any of the sensor prefixes */
    if (g_str_has_prefix(filename, "curr") ||
        (g_str_has_prefix(filename, "fan") &&
        !g_str_has_prefix(filename, "fan_div")) ||
        g_str_has_prefix(filename, "in") ||
        g_str_has_prefix(filename, "temp") ||
        g_str_has_prefix(filename, "vid")) {

        i2c_proc_plugin_add_sensor(sensors, path);
    }
    g_free(filename);

}

/* to be called to setup for proc sensors */
static GList *i2c_proc_plugin_init(void) {
    GList *sensors = NULL;

    /* call function to recursively look for sensors
    starting at the defined base directory */
    sensors_applet_plugin_find_sensors(&sensors, I2C_PROC_BASE_DIR, i2c_proc_plugin_test_sensor);
    return sensors;
}

static gdouble i2c_proc_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    /* to open and access the value of each sensor */
    FILE *fp;
    gfloat float1, float2, float3;
    gint int1, int2;

    gfloat sensor_value = -1.0;

    gchar *old_locale = NULL;

        /* always use C locale */
    if (NULL == (old_locale = setlocale(LC_NUMERIC, "C"))) {
        g_warning("Could not change locale to C locale for reading i2c-proc device files.. will try regardless");
    }

    if (NULL == (fp = fopen(path, "r"))) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_PROC_DEVICE_FILE_OPEN_ERROR, "Error opening sensor device file %s", path);
    } else {
        switch (type) {
            case CURRENT_SENSOR:
                if (fscanf(fp, "%f %f %f", &float1, &float2, &float3) != 3) {
                    g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_PROC_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                } else {
                    sensor_value = float3;
                }
                break;

            case FAN_SENSOR:
                if (fscanf(fp, "%d %d", &int1, &int2) != 2) {
                    g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_PROC_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                } else {
                    sensor_value = (gfloat)int2;
                }
                break;

            case VOLTAGE_SENSOR:
                /* is it CPU_VID or IN */
                switch(id[0]) {
                    case 'i':
                        if (fscanf(fp, "%f %f %f", &float1, &float2, &float3) != 3) {
                            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_PROC_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                        } else {
                            sensor_value = float3;
                        }
                        break;

                    case 'v':
                        /* want first value in file as float */
                        if (fscanf(fp, "%f", &float1) != 1) {
                            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_PROC_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                        } else {
                            sensor_value = float1;
                        }
                        break;
                    default:
                        g_debug("error in i2c-proc sensor read value function code - unhandled sensor id %s", id);
                        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_PROC_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                }
                break;

            case TEMP_SENSOR:
                if (fscanf(fp, "%f %f %f", &float1, &float2, &float3) != 3) {
                    g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_PROC_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                } else {
                    sensor_value = float3;
                }
                break;
        } /* end switch */
        fclose(fp);
    }

    if (NULL != old_locale) {
        setlocale(LC_NUMERIC, old_locale);
    }

    return (gdouble)sensor_value;
}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return i2c_proc_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return i2c_proc_plugin_get_sensor_value(path, id, type, error);
}
