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
#include "acpi-plugin.h"

const gchar *plugin_name = "acpi";

#define ACPI_THERMAL_ZONE_BASE_DIR "/proc/acpi/thermal_zone"
#define ACPI_THERMAL_BASE_DIR "/proc/acpi/thermal"

enum {
    ACPI_DEVICE_FILE_OPEN_ERROR,
    ACPI_DEVICE_FILE_READ_ERROR
};

static void acpi_plugin_add_sensor(GList **sensors, const gchar *path) {
    gchar *dirname;
    gchar *id;

    dirname = g_path_get_dirname(path);
    id = g_path_get_basename(dirname);
    g_free(dirname);

    sensors_applet_plugin_add_sensor(sensors,
                                     path,
                                     id,
                                     _("CPU"),
                                     TEMP_SENSOR,
                                     TRUE,
                                     CPU_ICON,
                                     DEFAULT_GRAPH_COLOR);
    g_free(id);
}

static void acpi_plugin_test_sensor(GList **sensors, const gchar *path) {
    gchar *filename;
    filename = g_path_get_basename(path);

    if (g_ascii_strcasecmp(filename, "temperature") == 0 ||
        g_ascii_strcasecmp(filename, "status") == 0) {
            acpi_plugin_add_sensor(sensors, path);
    }
    g_free(filename);
}

/* to be called to setup for acpi sensors and returns the list of found sensors */
static GList *acpi_plugin_init(void) {
    GList *sensors = NULL;

    /* call function to recursively look for sensors
       starting at the defined base directory */
    sensors_applet_plugin_find_sensors(&sensors, ACPI_THERMAL_ZONE_BASE_DIR, acpi_plugin_test_sensor);
    sensors_applet_plugin_find_sensors(&sensors, ACPI_THERMAL_BASE_DIR, acpi_plugin_test_sensor);
    return sensors;
}

static gdouble acpi_plugin_get_sensor_value(const gchar *path,
                                            const gchar *id,
                                            SensorType type,
                                            GError **error) {

    /* to open and access the value of each sensor */
    FILE *fp;
    gfloat sensor_value = -1.0f;
    gchar units[32];

    if (NULL == (fp = fopen(path, "r"))) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, ACPI_DEVICE_FILE_OPEN_ERROR, "Error opening sensor device file %s", path);
        return sensor_value;
    }

    if (fscanf(fp, "temperature: %f %31s", &sensor_value, units) < 1) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, ACPI_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
        fclose(fp);
        return sensor_value;
    }
    fclose(fp);

    /* need to convert if units are deciKelvin */
    if (g_ascii_strcasecmp(units, "dK") == 0) {
        sensor_value = (sensor_value / 10.0) - 273.0;
    }

    return (gdouble)sensor_value;
}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return acpi_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return acpi_plugin_get_sensor_value(path, id, type, error);
}
