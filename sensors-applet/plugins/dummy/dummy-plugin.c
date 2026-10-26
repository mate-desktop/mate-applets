/*
 * Copyright (C) 2018 info-cppsp <info@cppsp.de>
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

/* Dummy plugin to be able to test msa in a VM */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <time.h>
#include <stdlib.h>
#include "dummy-plugin.h"

// remove // from next line for syslog debug
//#define DPDBG              1

#ifdef DPDBG
#include <syslog.h>
#endif

const gchar *plugin_name = "dummy";

static void dummy_plugin_get_sensors(GList **sensors) {

    /* dummy HDD temp sensor */
    sensors_applet_plugin_add_sensor(sensors,
                                     "/sys/devices/platform/it87.656/hwmon/hwmon1/temp2_input",
                                     "temp2",
                                     "CPU",
                                     TEMP_SENSOR,
                                     TRUE,
                                     CPU_ICON,
                                     DEFAULT_GRAPH_COLOR);

    /* dummy HDD temp sensor */
    sensors_applet_plugin_add_sensor(sensors,
                                     "/sys/devices/platform/it87.656/hwmon/hwmon1/fan1_input",
                                     "fan1",
                                     "fan1",
                                     FAN_SENSOR,
                                     TRUE,
                                     FAN_ICON,
                                     DEFAULT_GRAPH_COLOR);

    /* dummy HDD temp sensor */
    sensors_applet_plugin_add_sensor(sensors,
                                     "HDD 2154884654-5648HG-546821",
                                     "Disk Temperature",
                                     "HDD 2154884654",
                                     TEMP_SENSOR,
                                     TRUE,
                                     HDD_ICON,
                                     DEFAULT_GRAPH_COLOR);

}

/* this is the function called every refresh cycle */
static gdouble dummy_plugin_get_sensor_value(const gchar *path,
                                             const gchar *id,
                                             SensorType type,
                                             GError **error) {

    switch (type) {
        case TEMP_SENSOR:
            return (gdouble) (rand() % 40 + 20);
            break;

        case FAN_SENSOR:
            return (gdouble) (rand() % 3000);
            break;

        default:
            g_assert_not_reached();
    }

}

/* API functions */
const gchar *sensors_applet_plugin_name(void) {
    return plugin_name;
}

static GList *dummy_plugin_init(void) {
    GList *sensors = NULL;

    /* init random number generation */
    srand(time(NULL));

    dummy_plugin_get_sensors(&sensors);

    return sensors;
}

GList *sensors_applet_plugin_init(void) {
    return dummy_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                               const gchar *id,
                                               SensorType type,
                                               GError **error) {

    return dummy_plugin_get_sensor_value(path, id, type, error);
}
