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
#include "i8k-plugin.h"

const gchar *plugin_name = "i8k";

#define I8K_DEVICE_PATH "/proc/i8k"
#define I8K_DEVICE_FILE "i8k"

enum {
    I8K_DEVICE_FILE_OPEN_ERROR,
    I8K_DEVICE_FILE_READ_ERROR
};

static void i8k_plugin_setup_manually(GList **sensors) {
    if (g_file_test(I8K_DEVICE_PATH, G_FILE_TEST_EXISTS)) {
        /* with i8k have only 3 fixed sensors, all accessed
           from the I8K_DEVICE_PATH */
        sensors_applet_plugin_add_sensor(sensors,
                                         I8K_DEVICE_PATH,
                                         "temp1",
                                         _("CPU"),
                                         TEMP_SENSOR,
                                         TRUE,
                                         CPU_ICON,
                                         DEFAULT_GRAPH_COLOR);

        sensors_applet_plugin_add_sensor(sensors,
                                         I8K_DEVICE_PATH,
                                         "fan1",
                                         _("FAN1"),
                                         FAN_SENSOR,
                                         FALSE,
                                         FAN_ICON,
                                         DEFAULT_GRAPH_COLOR);

        sensors_applet_plugin_add_sensor(sensors,
                                         I8K_DEVICE_PATH,
                                         "fan2",
                                         _("FAN2"),
                                         FAN_SENSOR,
                                         FALSE,
                                         FAN_ICON,
                                         DEFAULT_GRAPH_COLOR);
    }
}

/* to be called externally to setup for i8k sensors */
static GList *i8k_plugin_init(void) {
    GList *sensors = NULL;

    i8k_plugin_setup_manually(&sensors);

    return sensors;
}

static
gdouble i8k_plugin_get_sensor_value(const gchar *path,
                                    const gchar *id,
                                    SensorType type,
                                    GError **error) {

    /* to open and access the value of each sensor */
    FILE *fp;
    gint cpu_temp, fan1_status, fan2_status, fan1_rpm, fan2_rpm;
    gint sensor_value;
    gint space_count, file_length;

    if (NULL == (fp = fopen(path, "r"))) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I8K_DEVICE_FILE_OPEN_ERROR, "Error opening sensor device file %s", path);
        return -1.0;
    }

    space_count = 0;
    file_length = 0;

    /* count spaces but stop if have counted 100 characters and
       still not found a space (to avoid infinite loop if file
       format error) */
    while (file_length < 100 && space_count < 3) {
        if (fgetc(fp) == ' ') {
            space_count++;
        }
        file_length++;
    }

    if (fscanf(fp, "%d %d %d %d %d", &cpu_temp, &fan1_status, &fan2_status, &fan1_rpm, &fan2_rpm) != 5) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I8K_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
        fclose(fp);
        return -1.0;
    }
    fclose(fp);

    switch (type) {
        case TEMP_SENSOR:
            sensor_value = cpu_temp;
            break;

        case FAN_SENSOR:
            switch (id[3]) {
                case '1':
                    sensor_value = fan1_rpm;
                    break;

                case '2':
                    sensor_value = fan2_rpm;
                    break;

                default:
                    g_error("Error in i8k sensor get value function code for id %s", id);
                    g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I8K_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                    return -1.0;
            }
            break;

        default:
            g_error("Unknown sensor type passed as parameter to i8k sensor interface, cannot get value for this sensor");
            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, I8K_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
            return -1.0;
    } // end switch (sensor_type)

    return (gdouble)sensor_value;

}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return i8k_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return i8k_plugin_get_sensor_value(path, id, type, error);
}
