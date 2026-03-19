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
#include <glib.h>
#include <glib/gi18n.h>
#include "i2c-sys-plugin.h"

const gchar *plugin_name = "i2c-sys";

#define I2C_SYS_BASE_DIR "/sys"

enum {
    I2C_SYS_DEVICE_FILE_OPEN_ERROR,
    I2C_SYS_DEVICE_FILE_READ_ERROR
};

static void i2c_sys_plugin_add_sensor(GList **sensors, const gchar *path) {
    gchar *filename;
    gchar *id;
    gchar *label;
    gboolean enable;
    guint sensor_type;
    IconType icon_type = GENERIC_ICON;

    filename = g_path_get_basename(path);

    /* setup temp2 as CPU sensor and enable it */
    if (g_ascii_strcasecmp(filename, "temp2_input") == 0) {
        id = g_strndup(filename, 5);
        label = g_strdup(_("CPU"));
        sensor_type = TEMP_SENSOR;
        enable = TRUE;
        icon_type = CPU_ICON;
    } else {
        switch(filename[0]) {
            case 'c':    /* either current or cpu?_vid sensor */
                if (filename[1] == 'u') { /* currents are curr?_input */
                    id = g_strndup(filename, 5);
                    label = g_strndup(filename, 5);
                    sensor_type = CURRENT_SENSOR;
                } else { /* cpu_vid is cpu?_vid */
                    id = g_strdup(filename);
                    label = g_strdup(filename);
                    sensor_type = VOLTAGE_SENSOR;
                }
                break;

            case 'f':    /* fans are "fan?_input" */
                id = g_strndup(filename, 4);
                label = g_strndup(filename, 4);
                sensor_type = FAN_SENSOR;
                icon_type = FAN_ICON;
                break;

            case 't':    /* temps are "temp?_input" */
                id = g_strndup(filename, 5);
                label = g_strndup(filename, 5);
                sensor_type = TEMP_SENSOR;
                break;

            case 'i':    /* voltages are "in?_input" */
                id = g_strndup(filename, 3);
                label = g_strndup(filename, 3);
                sensor_type = VOLTAGE_SENSOR;
                break;

            default:
                /* SHOULDN'T BE ABLE * TO GET HERE!! */
                g_warning("filename:\"%s\" begins with a charater that is not covered by this switch statement... not adding sensor", filename);
                g_free(filename);

                return;
        }

        /* disable all other sensors */
        enable = FALSE;
    }

    sensors_applet_plugin_add_sensor(sensors,
                                         path,
                                         id,
                                         label,
                                         sensor_type,
                                         enable,
                                         icon_type,
                                         DEFAULT_GRAPH_COLOR);

    g_free(filename);
    g_free(id);
    g_free(label);
}

static void i2c_sys_plugin_test_sensor(GList **sensors, const gchar *path) {
gchar *filename;

filename = g_path_get_basename(path);

if ((g_str_has_suffix(filename, "_input") &&
    (g_str_has_prefix(filename, "temp") ||
    g_str_has_prefix(filename, "fan") ||
    g_str_has_prefix(filename, "curr") ||
    g_str_has_prefix(filename, "in"))) ||
    (g_str_has_prefix(filename, "cpu") &&
    (g_str_has_suffix(filename, "_vid")))) {

    i2c_sys_plugin_add_sensor(sensors, path);
}

    g_free(filename);
}

/* to be called to setup for sys sensors */
static GList *i2c_sys_plugin_init(void) {
    GList *sensors = NULL;

    /* call function to recursively look for sensors
       starting at the defined base directory */
    sensors_applet_plugin_find_sensors(&sensors,
                                       I2C_SYS_BASE_DIR,
                                       i2c_sys_plugin_test_sensor);
    return sensors;
}

static gdouble i2c_sys_plugin_get_sensor_value(const gchar *path,
                                               const gchar *id,
                                               SensorType type,
                                               GError **error) {

    /* to open and access the value of each sensor */
    FILE *fp;
    gfloat sensor_value;

    if (NULL == (fp = fopen(path, "r"))) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_SYS_DEVICE_FILE_OPEN_ERROR, "Error opening sensor device file %s", path);
        return -1.0;
    }

    if (fscanf(fp, "%f", &sensor_value) != 1) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I2C_SYS_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
        fclose(fp);
        return -1.0;
    }
    fclose(fp);

    if (type != FAN_SENSOR) {
        sensor_value /= 1000.0;
    }

    return (gdouble)sensor_value;
}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return i2c_sys_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return i2c_sys_plugin_get_sensor_value(path, id, type, error);
}
