/*
 * Copyright (C) 2006 Sven Peter <svpe@gmx.net>
 * Copyright (C) 2006 Alex Murray <murray.alex@gmail.com>
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

#include <X11/Xlib.h>
#include <NVCtrl/NVCtrl.h>
#include <NVCtrl/NVCtrlLib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include "nvidia-plugin.h"

const gchar *plugin_name = "nvidia";

#define THERMAL_SENSOR_TEMP "SensorTemp"
#define THERMAL_COOLER_LEVEL "CoolerLevel"
#define GPU_CORE_TEMP "CoreTemp"
#define AMBIENT_TEMP "AmbientTemp"

/* global variables */
Display *nvidia_sensors_dpy; /* the connection to the X server */

/* returns the value of the sensor */
static gdouble nvidia_plugin_get_sensor_value(const gchar *path,
                                              const gchar *id,
                                              SensorType type,
                                              GError **error) {

    Bool res;
    int temp;
    int i;

    i = g_ascii_strtoll(id + strlen("GPU"), NULL, 10);
    if (g_ascii_strcasecmp(path, THERMAL_SENSOR_TEMP) == 0) {
        res = XNVCTRLQueryTargetAttribute(nvidia_sensors_dpy,
                          NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
                          i,
                          0,
                          NV_CTRL_THERMAL_SENSOR_READING,
                          &temp);

    } else if (g_ascii_strcasecmp(path, THERMAL_COOLER_LEVEL) == 0) {
        res = XNVCTRLQueryTargetAttribute(nvidia_sensors_dpy,
                          NV_CTRL_TARGET_TYPE_COOLER,
                          i,
                          0,
                          NV_CTRL_THERMAL_COOLER_LEVEL,
                          &temp);

    } else if (g_ascii_strcasecmp(path, GPU_CORE_TEMP) == 0) {
        res = XNVCTRLQueryTargetAttribute(nvidia_sensors_dpy,
                          NV_CTRL_TARGET_TYPE_GPU,
                          i,
                          0,
                          NV_CTRL_GPU_CORE_TEMPERATURE,
                          &temp);

    } else if (g_ascii_strcasecmp(path, AMBIENT_TEMP) == 0) {
        res = XNVCTRLQueryTargetAttribute(nvidia_sensors_dpy,
                          NV_CTRL_TARGET_TYPE_GPU,
                          i,
                          0,
                          NV_CTRL_AMBIENT_TEMPERATURE,
                          &temp);

    } else {
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, 0, "Invalid path string passed to nvidia_plugin_get_sensor_value");
        return 0;
    }

    if (res != True) {
        /* when res isn't true something went wrong */
        g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, 0, "XNVCTRLQueryAttribute returned false");
        return 0;
    }

    /* convert the int to gdouble and return it */
    return (gdouble)temp;
}

/* creates the connection to the X server and checks whether the
 * NV-CONTROL extension is loaded */
static GList *nvidia_plugin_init(void) {
    int dummy;
    int event_base, error_base;
    GList *sensors = NULL;

    /* create the connection to the X server */
    if (!(nvidia_sensors_dpy = XOpenDisplay(NULL))) {
        /* no connection to the X server avaible */
        return sensors;
    }

    /* check if the NV-CONTROL extension is available on this X
         * server - if so add the two sensors if they exist */
    if (XNVCTRLQueryExtension(nvidia_sensors_dpy, &event_base, &error_base)) {
        int i, cnt;

        if (XNVCTRLQueryTargetCount(nvidia_sensors_dpy,
                        NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
                        &cnt)) {

            for (i = 0; i < cnt; i++) {
                gchar *id = g_strdup_printf("GPU%d%s", i, THERMAL_SENSOR_TEMP);

                sensors_applet_plugin_add_sensor(&sensors,
                                                 THERMAL_SENSOR_TEMP,
                                                 id,
                                                 _("GPU"),
                                                 TEMP_SENSOR,
                                                 TRUE,
                                                 GPU_ICON,
                                                 DEFAULT_GRAPH_COLOR);

                g_free(id);
            }
        }

        if (XNVCTRLQueryTargetCount(nvidia_sensors_dpy,
                        NV_CTRL_TARGET_TYPE_COOLER,
                        &cnt)) {

            for (i = 0; i < cnt; i++) {
                gchar *id = g_strdup_printf("GPU%d%s", i, THERMAL_COOLER_LEVEL);

                sensors_applet_plugin_add_sensor(&sensors,
                                                 THERMAL_COOLER_LEVEL,
                                                 id,
                                                 _("GPU"),
                                                 FAN_SENSOR,
                                                 TRUE,
                                                 FAN_ICON,
                                                 DEFAULT_GRAPH_COLOR);

                g_free(id);
            }
        }

        if (XNVCTRLQueryTargetCount(nvidia_sensors_dpy,
                        NV_CTRL_TARGET_TYPE_GPU,
                        &cnt)) {

            for (i = 0; i < cnt; i++) {
                if (XNVCTRLQueryTargetAttribute(nvidia_sensors_dpy,
                                NV_CTRL_TARGET_TYPE_GPU,
                                i,
                                       0, NV_CTRL_GPU_CORE_TEMPERATURE, &dummy)) {

                    gchar *id = g_strdup_printf("GPU%d%s", i, GPU_CORE_TEMP);

                    sensors_applet_plugin_add_sensor(&sensors,
                                                     GPU_CORE_TEMP,
                                                     id,
                                                     _("GPU"),
                                                     TEMP_SENSOR,
                                                     TRUE,
                                                     GPU_ICON,
                                                     DEFAULT_GRAPH_COLOR);
                    g_free(id);
                }

                if (XNVCTRLQueryTargetAttribute(nvidia_sensors_dpy,
                                NV_CTRL_TARGET_TYPE_GPU,
                                i,
                                       0, NV_CTRL_AMBIENT_TEMPERATURE, &dummy)) {
                    gchar *id = g_strdup_printf("GPU%d%s", i, AMBIENT_TEMP);

                    sensors_applet_plugin_add_sensor(&sensors,
                                                     AMBIENT_TEMP,
                                                     id,
                                                     _("Ambient"),
                                                     TEMP_SENSOR,
                                                     FALSE,
                                                     GENERIC_ICON,
                                                     DEFAULT_GRAPH_COLOR);
                    g_free(id);
                }
            }
        }

    }
    return sensors;
}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return nvidia_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return nvidia_plugin_get_sensor_value(path, id, type, error);
}

