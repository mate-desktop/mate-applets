/*
 * Copyright (C) 2017 info-cppsp <info@cppsp.de>
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

/*
Udisks2 plugin for the mate-sensors-applet

written using the structure and code of the previous version
from Pramod Dematagoda <pmd.lotr.gandalf@gmail.com>

dbus-glib documentation
https://dbus.freedesktop.org/doc/dbus-glib/
GDBUS documentation
https://developer.gnome.org/gio/stable/index.html

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <gio/gio.h>
#include "udisks2-plugin.h"

/* remove // from next line for syslog debug */
//#define UD2PD              1

#ifdef UD2PD
#include <syslog.h>
#endif

#define UDISKS2_BUS_NAME              "org.freedesktop.UDisks2"
#define UDISKS2_INTERFACE_NAME        "org.freedesktop.DBus.ObjectManager"
#define UDISKS2_DEVICE_INTERFACE_NAME "org.freedesktop.UDisks2.Drive"
#define UDISKS2_DEVICE_INTERFACE2_NAME "org.freedesktop.UDisks2.Drive.Ata"

#define UDISKS2_PROPERTIES_INTERFACE  "org.freedesktop.DBus.Properties"
#define UDISKS2_OBJECT_PATH           "/org/freedesktop/UDisks2"

/* Info about a single sensor */
typedef struct _DevInfo {
    gchar *path;
    gchar *id;
    gdouble temp;
    GDBusProxy *sensor_proxy;       /* dbus object */
    GError *error;
} DevInfo;

const gchar *plugin_name = "udisks2";

/* a container for the devices found to have smart enabled */
GHashTable *devices = NULL;

/* This is a global variable for convenience */
GDBusConnection *connection = NULL;

static void update_device (DevInfo *info) {
    GError *error = NULL;
    GVariant *tempgvar = NULL;
    GVariant *tempgvar2 = NULL;
    gdouble temp;

    /* check valid input parameter */
    g_return_if_fail (info != NULL);

    /* check connection too */
    g_return_if_fail (connection != NULL);

    g_clear_error (&info->error);

    /* check for sensor_proxy, which should exist at this point, make one if necessary and save it into DevInfo
     * this is used to get the temp value the direct way */
    if (NULL == info->sensor_proxy) {
        info->sensor_proxy = g_dbus_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                    UDISKS2_BUS_NAME,
                                                    info->path,
                                                    UDISKS2_PROPERTIES_INTERFACE,
                                                    NULL, &error);

        /* check, just to be sure */
        if (NULL == info->sensor_proxy) {

#ifdef UD2PD
syslog(LOG_ERR, "Failed to get drive temperature 1");
#endif
            g_debug ("Failed to get drive temperature 1: %s", error->message);
            g_clear_error (&error);
            return;
        }
    }

/* note on timing:
 * it seems to me that smart updates occur automatically every 10 minutes
 * mate-sensor-applet has a default refresh of 2 seconds...
 * it is possible to force a smart update with udisks2: SmartUpdate (IN  a{sv} options); */

    /* directly asking the device's DBus object for the temp */
    tempgvar = g_dbus_proxy_call_sync (info->sensor_proxy, "Get",
                g_variant_new ("(ss)",
                    UDISKS2_DEVICE_INTERFACE2_NAME,
                    "SmartTemperature"),                /* parameters */
                G_DBUS_CALL_FLAGS_NONE,                 /* flags */
                -1,                                     /* timeout */
                NULL,                                   /* cancellable */
                &error);

    if (NULL == tempgvar) {

#ifdef UD2PD
syslog(LOG_ERR, "Failed to get drive temperature 2");
#endif
        g_debug ("Failed to get drive temperature 2: %s", error->message);
        g_clear_error (&error);
        /* throw away proxy, maybe next time it will be better */
        g_clear_object (&info->sensor_proxy);
        return;

    } else {

#ifdef UD2PD
syslog(LOG_ERR, "tempgvar value: %s", g_variant_print(tempgvar, TRUE));
/* leaks memory! */
//syslog(LOG_ERR, "tempgvar value: %s", g_variant_print(g_variant_get_variant(g_variant_get_child_value(tempgvar, 0)), TRUE));
#endif

        /* tempgvar comes back as something along the lines of array(gvariant(tempasdouble))
         * hence unpacking
         * need to free up every param / return value, so can't do it like:
         * temp = g_variant_get_double(g_variant_get_variant(g_variant_get_child_value(tempgvar, 0))); */
        tempgvar2 = g_variant_get_child_value (tempgvar, 0);
        g_variant_unref (tempgvar);
        tempgvar = g_variant_get_variant (tempgvar2);
        g_variant_unref (tempgvar2);
        temp = g_variant_get_double (tempgvar);
        g_variant_unref (tempgvar);

        /* temp in K */
        info->temp = temp - 273.15;

#ifdef UD2PD
syslog(LOG_ERR, "Refresh udisks2 device temp: '%f'\n", info->temp);
#endif

    }

}

/* in this function we would like to get a list of device (hdd/ssd) paths
 * then with each path we get the temperature
 * it is possible with udisks2 to get all the above information in one g_dbus_proxy_call_sync(), so that is how I did it
 * maybe a better version would be to use GDBusObjectManager Server + Client ?? */
static void udisks2_plugin_get_sensors (GList **sensors) {

#ifdef UD2PD
syslog(LOG_ERR, "fstart");
#endif

    GDBusProxy *proxy = NULL;
    GError *error = NULL;

    DevInfo *info;

    /* This connection will be used for everything, including the obtaining of sensor data */
    connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
    if (NULL == connection) {

#ifdef UD2PD
syslog(LOG_ERR, "dbus conn fail");
#endif

        g_debug ("Failed to open connection to DBUS: %s", error->message);
        g_clear_error (&error);
        return;
    }

#ifdef UD2PD
syslog(LOG_ERR, "dbus conn success");
#endif

    /* I use this proxy to get all info of all devices at once */
    proxy = g_dbus_proxy_new_sync (connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                   UDISKS2_BUS_NAME,
                                   UDISKS2_OBJECT_PATH,
                                   UDISKS2_INTERFACE_NAME,
                                   NULL, &error);

    if (NULL == proxy) {

#ifdef UD2PD
syslog(LOG_ERR, "dbus conn proxy fail");
#endif
        g_debug ("dbus conn proxy fail: %s", error->message);
        g_clear_error (&error);
        g_clear_object (&connection);
        return;
    }

#ifdef UD2PD
syslog(LOG_ERR, "dbus conn proxy success");
#endif

    /* The object paths of the disks are enumerated and placed in an array of object paths
     * "GetManagedObjects" returns dict of (objectpath, (dict of (string [ie. if. name], dict of(string [ie. property name], variant [ie. prop. value])))) */

    /* g_dbus_proxy_call_sync() returns NULL on error, GVariant * otherwise
     * need second variable to prevent memory leak */
    GVariant *managed_objects = NULL;
    GVariant *managed_objects2 = NULL;

    managed_objects2 = g_dbus_proxy_call_sync (proxy, "GetManagedObjects",
                                               NULL,                           /* parameters */
                                               G_DBUS_CALL_FLAGS_NONE,         /* flags */
                                               -1,                             /* timeout */
                                               NULL,                           /* cancellable */
                                               &error);

    if (NULL == managed_objects2) {

#ifdef UD2PD
syslog(LOG_ERR, "Failed to enumerate disk devices");
#endif

        g_debug ("Failed to enumerate disk devices: %s", error->message);
        g_clear_error (&error);
        g_clear_object (&proxy);
        g_clear_object (&connection);
        return;
    }

    /* the result dictionary is enclosed in an array, unpack */
    managed_objects = g_variant_get_child_value (managed_objects2, 0);
    g_variant_unref (managed_objects2);

#ifdef UD2PD
//syslog(LOG_ERR, "managed_objects type: %s", g_variant_print(managed_objects, TRUE));
syslog(LOG_ERR, "success to enumerate disk devices");
#endif

    /* iterator for the result dictionary
     * iterator code is based on the g_variant_iter_next() documentation
     * iter is freed if the GVariant is, when using g_variant_iter_init() */
    GVariantIter iter;
    gchar *key = NULL;         /* object path (like '/org/freedesktop/UDisks2/drives/Samsung_SSD_840_EVO_250GB_*insert drive serial nr.*') */
    GVariant *value = NULL;

#ifdef UD2PD
/* log collection size */
syslog(LOG_ERR, "iter init count: %d", (int) g_variant_iter_init (&iter, managed_objects));
#else
    g_variant_iter_init (&iter, managed_objects);
#endif

    /* "{sv}"  is a GVariant format string
     * {} dictionary of, s string, v GVariant
     * changed to "{oa{sa{sv}}}" on error message 'the GVariant format string '{sv}' has a type of '{sv}' but the given value has a type of 'a{oa{sa{sv}}}''
     * a is array, o is object path
     * NOO!! the right format string is "{o@*}", which means get an object path into the 1st variable (key)
     * and get 'everything else' (as a GVariant) into the 2nd variable (value)
     * needs the & before the key and value params! */
    while (g_variant_iter_next (&iter, "{o@*}", &key, &value)) {

#ifdef UD2PD
syslog(LOG_ERR, "in iter while loop");
syslog(LOG_ERR, "key value: %s", key);
//syslog(LOG_ERR, "value type: %s", g_variant_print(value, TRUE));
#endif

        /* level 2
         * create a dictionary of value
         * the two interface names that we are searching for are known and defined
         * can't use GVariantDict, it only supports '{sv}' but the given value has a type of '{sa{sv}}'
         * using general lookup */

        GVariant *propdict = NULL;          /* drive data */
        GVariant *propdict2 = NULL;         /* drive smart data */

        /* make two dictionaries that contain the properties of the drive interfaces */
        propdict = g_variant_lookup_value (value, UDISKS2_DEVICE_INTERFACE_NAME, G_VARIANT_TYPE_DICTIONARY);
        propdict2 = g_variant_lookup_value (value, UDISKS2_DEVICE_INTERFACE2_NAME, G_VARIANT_TYPE_DICTIONARY);

        /* do we have the right ifname keys? */
        if ((NULL != propdict) && (NULL != propdict2)) {

#ifdef UD2PD
syslog(LOG_ERR, "propdict type: %s", g_variant_print(propdict, TRUE));
syslog(LOG_ERR, "propdict2 type: %s", g_variant_print(propdict2, TRUE));
#endif

            /* get data */
            const gchar *id = NULL;
            const gchar *model = NULL;

            gboolean smartenabled;
            gdouble temp;

            /* NULL, bc we don't care about the length of the string*/
            id = g_variant_get_string (g_variant_lookup_value (propdict, "Id", G_VARIANT_TYPE_STRING), NULL);
            model = g_variant_get_string (g_variant_lookup_value (propdict, "Model", G_VARIANT_TYPE_STRING), NULL);

            smartenabled = g_variant_get_boolean (g_variant_lookup_value (propdict2, "SmartEnabled", G_VARIANT_TYPE_BOOLEAN));
            temp = g_variant_get_double (g_variant_lookup_value (propdict2, "SmartTemperature", G_VARIANT_TYPE_DOUBLE));

#ifdef UD2PD
syslog(LOG_ERR, "Found udisks2 device id: '%s'\n", id);
syslog(LOG_ERR, "Found udisks2 device model: '%s'\n", model);
syslog(LOG_ERR, "Found udisks2 device smartenabled: '%d'\n", smartenabled);
syslog(LOG_ERR, "Found udisks2 device temp: '%f'\n", temp);
#endif

            /* only go on if smart is enabled
             * save data */
            if (smartenabled) {

                info = g_new0 (DevInfo, 1);
                if (NULL == devices) {
                    devices = g_hash_table_new (g_str_hash, g_str_equal);
                }

                info->id =  g_strdup (id);
                info->path =  g_strdup (key);

                /* temp in K
                 * this could be left at 0.0, 2 seconds later it will be refreshed anyway */
                info->temp = (gdouble) temp - 273.15;
                g_hash_table_insert (devices, info->id, info);

                /* Write the sensor data */
                sensors_applet_plugin_add_sensor (sensors,
                                                  id,
                                                  "Disk Temperature",
                                                  model,
                                                  TEMP_SENSOR,
                                                  FALSE,
                                                  HDD_ICON,
                                                  DEFAULT_GRAPH_COLOR);

                g_debug ("Added %s", id);

#ifdef UD2PD
syslog(LOG_ERR, "Added %s", id);
#endif

            } else {

#ifdef UD2PD
syslog(LOG_ERR, "No temp data for device: %s\n", key);
#endif

                g_debug ("No temp data for device: %s\n", key);
            }
        }

#ifdef UD2PD
syslog(LOG_ERR, "b4 free2");
#endif

        /* free propdict, propdict2
         * g_variant_dict_unref() may not work a few times, gives error
         * this one seems to do fine */
        if (NULL != propdict) {g_variant_unref (propdict);}
        if (NULL != propdict2) {g_variant_unref (propdict2);}

#ifdef UD2PD
syslog(LOG_ERR, "b4 free3");
#endif

        g_free (key);
        g_variant_unref (value);

    }       /* end of while loop */

    g_variant_unref (managed_objects);
    g_clear_object (&proxy);
    if (NULL == devices) {
        g_clear_object (&connection);
    }
}

/* this function is called every refresh cycle */
static gdouble udisks2_plugin_get_sensor_value (const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {
    DevInfo *info = NULL;

    /* get device stuct from data store */
    info = (DevInfo *) g_hash_table_lookup (devices, path);
    if (NULL == info) {
        g_set_error (error, SENSORS_APPLET_PLUGIN_ERROR, 0, "Error finding disk with path %s", path);
        return 0.0;
    }

    if (info->error) {
        *error = info->error;
        info->error = NULL;
        return 0.0;
    }

    /* refresh device temp */
    update_device (info);
    return info->temp;
}

/* API functions */
const gchar *sensors_applet_plugin_name (void) {
    return plugin_name;
}

static GList *udisks2_plugin_init (void) {
    GList *sensors = NULL;

    udisks2_plugin_get_sensors (&sensors);

    return sensors;
}

GList *sensors_applet_plugin_init (void) {
    return udisks2_plugin_init ();
}

gdouble sensors_applet_plugin_get_sensor_value (const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return udisks2_plugin_get_sensor_value (path, id, type, error);
}
