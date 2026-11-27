/*
 * Copyright (C) 2006-2008 Alex Murray <murray.alex@gmail.com>
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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib.h>
#include <glib/gi18n.h>
#include "sonypi-plugin.h"

#include <stdint.h>

const gchar *plugin_name = "sonypi";

/* These values are taken from spicctrl by Stelian Pop */
#define SONYPI_DEV       "/dev/sonypi"
#define SONYPI_IOCGFAN   _IOR('v', 10, uint8_t)
#define SONYPI_IOCGTEMP  _IOR('v', 12, uint8_t)
#define SONYPI_TEMP "sonypi_temp"

enum {
    SONYPI_DEVICE_FILE_OPEN_ERROR,
    SONYPI_DEVICE_FILE_READ_ERROR
};

static
GList *sonypi_plugin_init(void) {
    int fd;
    GList *sensors = NULL;

    if ( (fd = open(SONYPI_DEV, O_RDONLY)) != -1 ) {
        if ( close(fd) != -1 ) {
            sensors_applet_plugin_add_sensor(&sensors,
                                             SONYPI_DEV,
                                             SONYPI_TEMP,
                                             _("CPU TEMP"),
                                             TEMP_SENSOR,
                                             TRUE,
                                             CPU_ICON,
                                             DEFAULT_GRAPH_COLOR);
        }
    }
    return sensors;
}

static
gdouble sonypi_plugin_get_sensor_value(const gchar *path,
                                       const gchar *id,
                                       SensorType type,
                                       GError **error) {

    int fd;
    guint8 value8;

    gdouble sensor_value = -1.0;

    if ((fd = open(path, O_RDONLY)) != -1) {
        /* only use temp sensor */
        if (g_ascii_strcasecmp(id, SONYPI_TEMP) == 0) {
            if (ioctl(fd, SONYPI_IOCGTEMP, &value8) != -1) {
                sensor_value = (gdouble)value8;
            } else {
                g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, SONYPI_DEVICE_FILE_READ_ERROR, "Error reading from sensor device file %s", path);
            }
        }
        close(fd);
    } else {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, SONYPI_DEVICE_FILE_OPEN_ERROR, "Error opening from sensor device file %s", path);
    }

    return sensor_value;
}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return sonypi_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return sonypi_plugin_get_sensor_value(path, id, type, error);
}
