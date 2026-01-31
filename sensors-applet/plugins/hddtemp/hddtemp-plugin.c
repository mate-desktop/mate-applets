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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>
#include <glib.h>
#include "hddtemp-plugin.h"

const gchar *plugin_name = "hddtemp";

#define HDDTEMP_SERVER_IP_ADDRESS "127.0.0.1"
#define HDDTEMP_PORT_NUMBER 7634
#define HDDTEMP_OUTPUT_BUFFER_LENGTH 1024

enum {
    HDDTEMP_SOCKET_OPEN_ERROR,
    HDDTEMP_SOCKET_CONNECT_ERROR,
    HDDTEMP_GIOCHANNEL_ERROR,
    HDDTEMP_GIOCHANNEL_READ_ERROR
};

static gchar buffer[HDDTEMP_OUTPUT_BUFFER_LENGTH];

static const gchar *hddtemp_plugin_query_hddtemp_daemon(GError **error) {
    guint output_length = 0;
    static gboolean first_run = TRUE;

    struct sockaddr_in address;
    static gint64 previous_query_time;
    gint64 current_query_time;

    if (first_run) {
        // initialise previous time
        previous_query_time = g_get_monotonic_time ();
    }
    current_query_time = g_get_monotonic_time ();

    /* only actually query if more than 60 seconds has elapsed as
    hddtemp daemon will only actually send a new value if is > 60
    seconds */
    if (first_run || current_query_time - previous_query_time > G_TIME_SPAN_MINUTE) {
        int sockfd;
        ssize_t n;
        gchar *pc;

        previous_query_time = current_query_time;

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            // couldn't open socket
            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, HDDTEMP_SOCKET_OPEN_ERROR, "Error opening socket for hddtemp");
            return NULL;
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(HDDTEMP_SERVER_IP_ADDRESS);
        address.sin_port = htons(HDDTEMP_PORT_NUMBER);

        if (connect(sockfd, (struct sockaddr *)&address, (socklen_t)sizeof(address)) == -1) {
            g_set_error(error, SENSORS_APPLET_PLUGIN_ERROR, HDDTEMP_SOCKET_CONNECT_ERROR, "Error connecting to hddtemp daemon on port %i on %s", htons(HDDTEMP_PORT_NUMBER), HDDTEMP_SERVER_IP_ADDRESS);
            close(sockfd);
            return NULL;
        }
        memset(buffer, 0, sizeof(buffer));
        pc = buffer;
        while ((n = read(sockfd, pc, sizeof(buffer) - output_length)) > 0) {
            output_length += n;
            pc += n;
        }
        /* always null terminate the end of the buffer */
        buffer[MIN(output_length, sizeof(buffer) - 1)] = '\0';
        close(sockfd);
        first_run = FALSE;
    }

    return buffer;
}

static void hddtemp_plugin_get_sensors(GList **sensors) {
    GError *error = NULL;
    const gchar *hddtemp_output;
    gchar **output_vector = NULL, **pv;

    hddtemp_output = hddtemp_plugin_query_hddtemp_daemon(&error);

    if (error) {
        g_error_free(error);
        return;
    }

    if (hddtemp_output[0] != '|') {
        g_debug("Error in format of string returned from hddtemp daemon: char at [0] should be \"|\", instead whole output is: \"%s\"", hddtemp_output);
        return;
    }

    /* for each sensor the output will contain four strings ie
       |/dev/hda|WDC WD800JB-00ETA0|32|C||/dev/hdb|???|ERR|*|
       note the repetition -----^ */

    /*
      pv[0 + 5*n] empty
      pv[1 + 5*n] device name
      pv[2 + 5*n] disk label
      pv[3 + 5*n] temperature
      pv[4 + 5*n] unit
      pv[5 + 5*n] empty
    */

    pv = output_vector = g_strsplit(hddtemp_output, "|", -1);

    while(pv[1] != NULL) {
        if (g_strcmp0(pv[2], "") != 0 &&
            g_strcmp0(pv[3], "") != 0 &&
            g_strcmp0(pv[4], "") != 0 &&
            (!(g_ascii_strcasecmp(pv[2], "???") == 0 ||
               g_ascii_strcasecmp(pv[3], "ERR") == 0 ||
               g_ascii_strcasecmp(pv[4], "*") == 0))) {

            sensors_applet_plugin_add_sensor(sensors,
                                            pv[1], // must be dynamically allocated
                                            pv[1], // must be dynamically allocated
                                            pv[2], // must be dynamically allocated
                                            TEMP_SENSOR,
                                            FALSE,
                                            HDD_ICON,
                                            DEFAULT_GRAPH_COLOR);
        }
        pv += 5;
    }
    g_strfreev(output_vector);
}

/* to be called to setup for hddtemp sensors */
static GList *hddtemp_plugin_init(void) {
    GList *sensors = NULL;
    hddtemp_plugin_get_sensors(&sensors);
    return sensors;
}

/* returns the value of the sensor_list at the given iter, or if an
   error occurs, instatiates error with an error message */
static gdouble hddtemp_plugin_get_sensor_value(const gchar *path,
                                              const gchar *id,
                                              SensorType type,
                                              GError **error) {

    const gchar *hddtemp_output;
    gchar **output_vector = NULL, **pv;

    gfloat sensor_value = -1.0f;

    hddtemp_output = hddtemp_plugin_query_hddtemp_daemon(error);

    if (*error) {
        return sensor_value;
    }

    if (hddtemp_output[0] != '|') {
        g_debug("Error in format of string returned from hddtemp daemon: char at [0] should be \"|\", instead whole output is: \"%s\"", hddtemp_output);
        return sensor_value;
    }

    /* for each sensor the output will contain four strings ie
       |/dev/hda|WDC WD800JB-00ETA0|32|C||/dev/hdb|???|ERR|*|
                note the repetition -----^

      pv[0 + 5*n] empty
      pv[1 + 5*n] device name
      pv[2 + 5*n] disk label
      pv[3 + 5*n] temperature
      pv[4 + 5*n] unit
      pv[5 + 5*n] empty
    */

    pv = output_vector = g_strsplit(hddtemp_output, "|", -1);

    while(pv[1] != NULL) {
        if (g_ascii_strcasecmp(pv[1], path) == 0) {
            sensor_value = (gfloat)(g_ascii_strtod(pv[3], NULL));

            /* always return sensor values in celsius */
            if (pv[4][0] == 'F') {
                sensor_value = (sensor_value - 32.0) * 5.0 / 9.0;
            }
            break;
        }
        pv += 5;
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
    return hddtemp_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {
    return hddtemp_plugin_get_sensor_value(path, id, type, error);
}
