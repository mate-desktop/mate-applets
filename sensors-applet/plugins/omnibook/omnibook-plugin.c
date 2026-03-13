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
#include "omnibook-plugin.h"

const gchar *plugin_name = "omnibook";

#define OMNIBOOK_DEVICE_FILE "/proc/omnibook/temperature"

enum {
    OMNIBOOK_DEVICE_FILE_OPEN_ERROR,
    OMNIBOOK_DEVICE_FILE_READ_ERROR
};

static void omnibook_plugin_setup_manually(GList **sensors) {
    /* omnibook only has the one device file with one temp in it */
    if (g_file_test(OMNIBOOK_DEVICE_FILE, G_FILE_TEST_EXISTS)) {

        sensors_applet_plugin_add_sensor(sensors,
                                         OMNIBOOK_DEVICE_FILE,
                                         _("temperature"),
                                         _("CPU"),
                                         TEMP_SENSOR,
                                         TRUE,
                                         CPU_ICON,
                                         DEFAULT_GRAPH_COLOR);
    }
}

/* to be called to setup for sys sensors */
static GList *omnibook_plugin_init(void) {
    GList *sensors = NULL;
    /* call function to recursively look for sensors starting at the defined base directory */
    omnibook_plugin_setup_manually(&sensors);

    return sensors;
}

static gdouble omnibook_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    /* to open and access the value of each sensor */
    FILE *fp;
    gfloat sensor_value;

    if (NULL == (fp = fopen(path, "r"))) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, OMNIBOOK_DEVICE_FILE_OPEN_ERROR, "Error opening sensor device file %s", path);
        return -1.0;
    }

    if (fscanf(fp, "CPU temperature: %f", &sensor_value) != 1) {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, OMNIBOOK_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
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
    return omnibook_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return omnibook_plugin_get_sensor_value(path, id, type, error);
}
