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
#include "ibm-acpi-plugin.h"

const gchar *plugin_name = "ibm-acpi";

#define IBM_ACPI_TEMPERATURE_FILE "/proc/acpi/ibm/thermal"
#define IBM_ACPI_FAN_FILE "/proc/acpi/ibm/fan"

/* sensor id's */
#define CPU "CPU"
#define MPCI "mPCI"
#define HDD "HDD"
#define GPU "GPU"
#define BAT0 "BAT0"
#define BAT1 "BAT1"
#define FAN "FAN"

enum {
    IBM_ACPI_FILE_OPEN_ERROR,
    IBM_ACPI_FILE_READ_ERROR
};

static void ibm_acpi_plugin_setup_manually(GList **sensors) {
    if (g_file_test(IBM_ACPI_TEMPERATURE_FILE, G_FILE_TEST_EXISTS)) {
        gchar *label;

        /* with Ibm_Acpi have 8 fixed sensors, all accessed
           from the IBM_ACPI_TEMPERATURE_FILE */
        sensors_applet_plugin_add_sensor(sensors,
                                         IBM_ACPI_TEMPERATURE_FILE,
                                         CPU,
                                         _(CPU),
                                         TEMP_SENSOR,
                                         TRUE,
                                         CPU_ICON,
                                         DEFAULT_GRAPH_COLOR);

        sensors_applet_plugin_add_sensor(sensors,
                                         IBM_ACPI_TEMPERATURE_FILE,
                                         MPCI,
                                         _("MiniPCI"),
                                         TEMP_SENSOR,
                                         FALSE,
                                         GENERIC_ICON,
                                         DEFAULT_GRAPH_COLOR);

        sensors_applet_plugin_add_sensor(sensors,
                                         IBM_ACPI_TEMPERATURE_FILE,
                                         HDD,
                                         _(HDD),
                                         TEMP_SENSOR,
                                         FALSE,
                                         HDD_ICON,
                                         DEFAULT_GRAPH_COLOR);

        sensors_applet_plugin_add_sensor(sensors,
                                         IBM_ACPI_TEMPERATURE_FILE,
                                         GPU,
                                         _(GPU),
                                         TEMP_SENSOR,
                                         FALSE,
                                         GPU_ICON,
                                         DEFAULT_GRAPH_COLOR);

        label = g_strdup_printf("%s %d", _("Battery"), 0);
        sensors_applet_plugin_add_sensor(sensors,
                                         IBM_ACPI_TEMPERATURE_FILE,
                                         BAT0,
                                         label,
                                         TEMP_SENSOR,
                                         FALSE,
                                         BATTERY_ICON,
                                         DEFAULT_GRAPH_COLOR);

        g_free(label);

        label = g_strdup_printf("%s %d", _("Battery"), 0);
        sensors_applet_plugin_add_sensor(sensors,
                                         IBM_ACPI_TEMPERATURE_FILE,
                                         BAT1,
                                         label,
                                         TEMP_SENSOR,
                                         FALSE,
                                         BATTERY_ICON,
                                         DEFAULT_GRAPH_COLOR);

        g_free(label);
    }

    if (g_file_test(IBM_ACPI_FAN_FILE, G_FILE_TEST_EXISTS)) {
        sensors_applet_plugin_add_sensor(sensors,
                                         IBM_ACPI_FAN_FILE,
                                         FAN,
                                         _("Fan"),
                                         FAN_SENSOR,
                                         FALSE,
                                         FAN_ICON,
                                         DEFAULT_GRAPH_COLOR);

    }
}

/* to be called externally to setup for ibm_acpi sensors */
static
GList *ibm_acpi_plugin_init(void) {
    GList *sensors = NULL;

    ibm_acpi_plugin_setup_manually(&sensors);

    return sensors;
}

static
gdouble ibm_acpi_plugin_get_sensor_value(const gchar *path,
                                         const gchar *id,
                                         SensorType type,
                                         GError **error) {

    /* to open and access the value of each sensor */
    FILE *fp;
    gint sensor_value = 0;
    gint cpu_temp, minipci_temp, hdd_temp, gpu_temp, bat0_temp, bat1_temp, unknown0, unknown1;
    gint fan_speed;

    if (NULL == (fp = fopen(path, "r"))) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, IBM_ACPI_FILE_OPEN_ERROR, "Error opening sensor device file %s", path);
        return -1.0;
    }

    switch (type) {
        case TEMP_SENSOR:
            if (fscanf(fp, "temperatures:   %d %d %d %d %d %d %d %d", &cpu_temp, &minipci_temp, &hdd_temp, &gpu_temp, &bat0_temp, &unknown0, &bat1_temp, &unknown1) != 8) {
                g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, IBM_ACPI_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                fclose(fp);
                return -1.0;
            }

            if (g_ascii_strcasecmp(id, CPU) == 0) {
                sensor_value = cpu_temp;
            } else if (g_ascii_strcasecmp(id, MPCI) == 0) {
                sensor_value = minipci_temp;
            } else if (g_ascii_strcasecmp(id, HDD) == 0) {
                sensor_value = hdd_temp;
            } else if (g_ascii_strcasecmp(id, GPU) == 0) {
                sensor_value = gpu_temp;
            } else if (g_ascii_strcasecmp(id, BAT0) == 0) {
                sensor_value = bat0_temp;
            } else if (g_ascii_strcasecmp(id, BAT1) == 0) {
                sensor_value = bat1_temp;
            }
            break;

        case FAN_SENSOR:
            /*fscanf(fp, "%*[^\n]");*/   /* Skip to the End of the Line */
            /*fscanf(fp, "%*1[\n]");*/   /* Skip One Newline */
            if (fscanf(fp, "%*[^\n]%*1[\n]speed:   %d", &fan_speed) != 1) {
                g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, IBM_ACPI_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
                fclose(fp);
                return -1.0;
            }

            if (g_ascii_strcasecmp(id, "FAN") == 0) {
                sensor_value = fan_speed;
            }
            break;

        default:
            g_error("Unknown sensor type passed as parameter to ibm-acpi sensor interface, cannot get value for this sensor");
            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, IBM_ACPI_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
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
    return ibm_acpi_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return ibm_acpi_plugin_get_sensor_value(path, id, type, error);
}
