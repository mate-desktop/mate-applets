/*
 * Copyright (C) 2010 Daniele Napolitano <dnax88@gmail.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <unistd.h>
#include "mbmon-plugin.h"

const gchar *plugin_name = "mbmon";

#define MBMON_SERVER_IP_ADDRESS "127.0.0.1"
#define MBMON_PORT_NUMBER 411
#define MBMON_OUTPUT_BUFFER_LENGTH 1024

enum {
    MBMON_SOCKET_OPEN_ERROR,
    MBMON_SOCKET_CONNECT_ERROR,
    MBMON_GIOCHANNEL_ERROR,
    MBMON_GIOCHANNEL_READ_ERROR
};

static const gchar *mbmon_plugin_query_mbmon_daemon(GError **error) {
    static gboolean first_run = FALSE;
    gint output_length = 0;
    gchar *pc;

    struct sockaddr_in address;
    static char* buffer = NULL;
    static gint64 previous_query_time;
    gint64 current_query_time;

    if (NULL == buffer) {
        /* initialise buffer and current time */
        buffer = g_new0(char, MBMON_OUTPUT_BUFFER_LENGTH);
        previous_query_time = g_get_monotonic_time ();
        first_run = TRUE;
    }
    current_query_time = g_get_monotonic_time ();

    /* only query if more than 2 seconds has elapsed,
    mbmon daemon will send a new value every 2 seconds */
    if (first_run || current_query_time - previous_query_time > 2 * G_TIME_SPAN_SECOND) {
        int sockfd;
        ssize_t n;

        previous_query_time = current_query_time;

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            /* couldn't open socket */
            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, MBMON_SOCKET_OPEN_ERROR, "Error opening socket for mbmon");
            return NULL;
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(MBMON_SERVER_IP_ADDRESS);
        address.sin_port = htons(MBMON_PORT_NUMBER);

        if (connect(sockfd, (struct sockaddr *)&address, (socklen_t)sizeof(address)) == -1) {
            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, MBMON_SOCKET_CONNECT_ERROR, "Error connecting to mbmon daemon on port %i on %s", htons(MBMON_PORT_NUMBER), MBMON_SERVER_IP_ADDRESS);
            return NULL;
        }

        pc = buffer;
        while ((n = read(sockfd, pc, MBMON_OUTPUT_BUFFER_LENGTH - output_length)) > 0) {
            output_length += n;
            pc = &pc[n];
        }
        /* always null terminate the end of the buffer
         * regardless of how much stuff is in it already */
        buffer[output_length] = '\0';
        close(sockfd);
    }

    return buffer;
}

static SensorType get_sensor_type (const char *name) {
    SensorType type = CURRENT_SENSOR;

    if (g_strrstr(name, "FAN")) {
        type = FAN_SENSOR;
    } else if (g_strrstr(name, "TEMP")) {
        type = TEMP_SENSOR;
    } else {
        type = VOLTAGE_SENSOR;
    }
    return type;
}

static IconType get_sensor_icon (SensorType type) {
    switch (type) {
        case TEMP_SENSOR:
            return CPU_ICON;
        case FAN_SENSOR:
            return FAN_ICON;
        default:
            return GENERIC_ICON;
    }
}

static void mbmon_plugin_get_sensors(GList **sensors) {

    GError *error = NULL;
    const gchar *mbmon_output;
    gchar **output_vector = NULL, **pv;

    mbmon_output = mbmon_plugin_query_mbmon_daemon(&error);

    if (error) {
        g_error_free(error);
        return;
    }

    pv = output_vector = g_strsplit(mbmon_output, "\n", -1);

    while(pv[0] != NULL) {
        gchar **pv2;

        pv2 = g_strsplit(pv[0], ":", -1);
        gchar *name, *label;
        SensorType type;
        gboolean visible;
        IconType icon;
        gdouble low_value, high_value;

        type = get_sensor_type(pv2[0]);
        icon = get_sensor_icon(type);
        name = g_strstrip(pv2[0]);
        label = NULL;
        visible = (type == TEMP_SENSOR ? TRUE : FALSE);

        if(type == VOLTAGE_SENSOR) {
            if(g_strrstr(name, "VC0")) {
                label = g_strdup("Core Voltage 1");
            } else if(g_strrstr(name, "VC1")) {
                label = g_strdup("Core Voltage 2");
            } else if(g_strrstr(name, "V33")) {
                label = g_strdup("+3.3v Voltage");
            } else if(g_strrstr(name, "V50P")) {
                label = g_strdup("+5v Voltage");
            } else if(g_strrstr(name, "V12P")) {
                label = g_strdup("+12v Voltage");
            } else if(g_strrstr(name, "V12N")) {
                label = g_strdup("-12v Voltage");
            } else if(g_strrstr(name, "V50N")) {
                label = g_strdup("-5v Voltage");
            }
        }

        label = (label == NULL ? name : label);

        sensors_applet_plugin_default_sensor_limits(type, &low_value, &high_value);

        sensors_applet_plugin_add_sensor_with_limits(sensors,
                                                     name,
                                                     name,
                                                     label,
                                                     type,
                                                     visible,
                                                     low_value,
                                                     high_value,
                                                     icon,
                                                     DEFAULT_GRAPH_COLOR);

        g_strfreev(pv2);
        pv++;
    }
    g_strfreev(output_vector);

}

/* to be called to setup for mbmon sensors */
static GList *mbmon_plugin_init(void) {
    GList *sensors = NULL;
    mbmon_plugin_get_sensors(&sensors);
    return sensors;
}

/* returns the value of the sensor_list at the given iter, or if an
   error occurs, instatiates error with an error message */
static gdouble mbmon_plugin_get_sensor_value(const gchar *path,
                                            const gchar *id,
                                            SensorType type,
                                            GError **error) {

    const gchar *mbmon_output;
    gchar **output_vector = NULL, **pv, **pv2;

    gfloat sensor_value = -1.0f;

    mbmon_output = mbmon_plugin_query_mbmon_daemon(error);

    if (*error) {
        return sensor_value;
    }

    pv = output_vector = g_strsplit(mbmon_output, "\n", -1);

    while(pv[0] != NULL) {
        if (g_strrstr(pv[0], path) != NULL) {

            pv2 = g_strsplit(pv[0], ":", -1);
            sensor_value = (gfloat)(g_ascii_strtod(pv2[1], NULL));
            g_strfreev(pv2);
            break;
        }
        pv++;
    }
    g_strfreev(output_vector);

    return (gdouble)sensor_value;
}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return mbmon_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return mbmon_plugin_get_sensor_value(path, id, type, error);
}
