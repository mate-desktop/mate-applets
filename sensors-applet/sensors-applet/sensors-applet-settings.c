/*
 * Copyright (C) 2005-2009 Alex Murray <murray.alex@gmail.com>
 *               2013 Stefano Karapetsas <stefano@karapetsas.com>
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

#include <string.h>
#include <glib.h>
#include "sensors-applet.h"
#include "sensors-applet-settings.h"

/* #define SORT_DEBUG */
#ifdef SORT_DEBUG
#include <syslog.h>
#endif

/* these must be as the gsettings schema in file org.mate.sensors-applet.gschema.xml.in
 * gsettings gvariant type string */
#define GSGVTS "s"
/* gsettings gvariant type string for the array */
#define GSGVTSA "as"

gchar* sensors_applet_settings_get_unique_id (const gchar *interface, const gchar *id, const gchar *path) {
    gchar *unique_id = NULL;
    gchar *unique_id_hash = NULL;
    GChecksum *checksum = NULL;
    guint8 digest[16];
    gsize digest_len = sizeof (digest);

    unique_id = g_strdup_printf ("%s/%s/%s", interface, id, path);

    checksum = g_checksum_new (G_CHECKSUM_MD5);
    g_checksum_update (checksum, (const guchar *) unique_id, strlen (unique_id));
    g_checksum_get_digest (checksum, digest, &digest_len);
    g_assert (digest_len == 16);

    unique_id_hash = g_strdup (g_checksum_get_string (checksum));

    g_checksum_free (checksum);
    g_free (unique_id);

    return unique_id_hash;
}

/* load sensors from gsettings with sorting */
gboolean sensors_applet_settings_load_sensors (SensorsApplet *sensors_applet) {
    /* everything gets stored except alarm timeout indexes, which
    we set to -1, and visible which we set to false for all
    parent nodes and true for all child nodes */

    gchar *applet_path = NULL;
    /* not sure about pointer, it is unclear if it is freed by loop, probably yes */
    GVariantIter *iter = NULL;
    gchar *gsuid = NULL;

    /* string variables, to help free up memory in loop */
    gchar *current_path = NULL;
    gchar *current_id = NULL;
    gchar *current_label = NULL;
    gchar *current_interface = NULL;
    gchar *current_low_alarm_command = NULL;
    gchar *current_high_alarm_command = NULL;
    gchar *current_graph_color = NULL;

    /* get gsettings path for applet */
    applet_path = mate_panel_applet_get_preferences_path (sensors_applet->applet);

    /* get sensors-list array from gsettings */
    g_settings_get (sensors_applet->settings, "sensors-list", GSGVTSA, &iter);

    /* load sensor data into applet one by one
     * first get unique id for the sensor
     * then load sensor data from gsettings
     * then add sensor
     * gsuid is freed by g_variant_iter_loop() */
    while (g_variant_iter_loop (iter, GSGVTS, &gsuid)) {

        /* load sensor data from gsettings individually
         * g_strdup_printf doesn't free args!
         * applet_path is freed at the end */
        gchar *path = g_strdup_printf ("%s%s/", applet_path, gsuid);
        /*g_free (gsuid);   // freed by loop */

        /* make a schema which points to one sensor data */
        GSettings *settings;
        settings = g_settings_new_with_path ("org.mate.sensors-applet.sensor", path);
        g_free (path);

        /* laod sensor data to temp variables
         * g_settings_get_string allocates memory!
         * sensors_applet_add_sensor() gtk_tree_store_set() copies strings, so free them */
        sensors_applet_add_sensor(sensors_applet,
            current_path = g_settings_get_string (settings, PATH),
            current_id = g_settings_get_string (settings, ID),
            current_label = g_settings_get_string (settings, LABEL),
            current_interface = g_settings_get_string (settings, INTERFACE),
            g_settings_get_int (settings, SENSOR_TYPE),
            g_settings_get_boolean (settings, ENABLED),
            g_settings_get_double (settings, LOW_VALUE),
            g_settings_get_double (settings, HIGH_VALUE),
            g_settings_get_boolean (settings, ALARM_ENABLED),
            current_low_alarm_command = g_settings_get_string (settings, LOW_ALARM_COMMAND),
            current_high_alarm_command = g_settings_get_string (settings, HIGH_ALARM_COMMAND),
            g_settings_get_int (settings, ALARM_TIMEOUT),
            g_settings_get_double (settings, MULTIPLIER),
            g_settings_get_double (settings, OFFSET),
            g_settings_get_int (settings, ICON_TYPE),
            current_graph_color = g_settings_get_string (settings, GRAPH_COLOR));

        g_free (current_path);
        g_free (current_id);
        g_free (current_label);
        g_free (current_interface);
        g_free (current_low_alarm_command);
        g_free (current_high_alarm_command);
        g_free (current_graph_color);

        g_object_unref (settings);

    }

    g_free (applet_path);

    return TRUE;
}

#ifdef SORT_DEBUG
/* print a gtk_tree_store (for debug) */
static void sensors_applet_settings_print_sensors_tree (SensorsApplet *sensors_applet) {

    g_assert(sensors_applet);
    g_assert(sensors_applet->sensors);

    GtkTreeIter interfaces_iter;
    GtkTreeIter sensors_iter;
    gboolean not_end_of_interfaces = TRUE;
    gboolean not_end_of_sensors = TRUE;
    gint interfaces_counter = 0;
    gint sensors_counter = 0;

    gchar *interface_name = NULL;
    gchar *sensor_id = NULL;
    gchar *sensor_path = NULL;
    gchar *sensor_hash = NULL;

    /* iterate through the sensor tree
     * code from sensors-applet.c sensors_applet_add_sensor()
     * first go through the interfaces */
    for (not_end_of_interfaces = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter);
        not_end_of_interfaces;
        not_end_of_interfaces = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter), interfaces_counter++) {

        /* get interface name */
        gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter, INTERFACE_COLUMN, &interface_name, -1);

        /* print interface name */
        syslog(LOG_ERR, "#%d interface name: %s\n", interfaces_counter, interface_name);

        /* then go through the individual sensors under one interface */
        for (not_end_of_sensors = gtk_tree_model_iter_children(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter,  &interfaces_iter);
            not_end_of_sensors;
            not_end_of_sensors = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter), sensors_counter++) {

            gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter,
                       PATH_COLUMN, &sensor_path,
                       ID_COLUMN, &sensor_id,
                       -1);

            sensor_hash = sensors_applet_settings_get_unique_id (interface_name, sensor_id, sensor_path);

            /* print sensor data */
            syslog(LOG_ERR, "\t#%d sensor id: %s\n", sensors_counter, sensor_id);
            syslog(LOG_ERR, "\t#%d sensor path: %s\n", sensors_counter, sensor_path);
            syslog(LOG_ERR, "\t#%d sensor hash: %s\n\n", sensors_counter, sensor_hash);

            g_free(sensor_id);
            g_free(sensor_path);
            g_free(sensor_hash);

        }

        g_free(interface_name);

    }

}
#endif

/* compare two iters using their paths
 * kinda same as active_sensor_compare () */
static gint sensors_applet_settings_sort_sensors_iter_compare (SensorsApplet *sensors_applet, GtkTreeIter ti_a, GtkTreeIter ti_b) {

    GtkTreePath *tp_a;
    GtkTreePath *tp_b;
    gint ret_val;

    tp_a = gtk_tree_model_get_path (GTK_TREE_MODEL(sensors_applet->sensors), &ti_a);
    tp_b = gtk_tree_model_get_path (GTK_TREE_MODEL(sensors_applet->sensors), &ti_b);

    ret_val = gtk_tree_path_compare(tp_a, tp_b);

    gtk_tree_path_free(tp_a);
    gtk_tree_path_free(tp_b);

    return ret_val;
}

/* find two sensors in the sensors tree based on their hash and sort them
 * Return codes:
 * 0: something went very wrong...
 * 1: normal run, a and b are in place
 * 2: hash_b has already been removed from the system
 * 3: hash_a is not found */
static gint sensors_applet_settings_sort_sensors_sort (SensorsApplet *sensors_applet,
                                                           const gchar *hash_a,
                                                           const gchar *hash_b,
                                                           gboolean a_is_first) {

    g_assert(sensors_applet);
    g_assert(sensors_applet->sensors);

    GtkTreeIter interfaces_iter;
    GtkTreeIter sensors_iter;
    gboolean not_end_of_interfaces = TRUE;
    gboolean not_end_of_sensors = TRUE;

    gchar *interface_name = NULL;
    gchar *sensor_id = NULL;
    gchar *sensor_path = NULL;
    gchar *sensor_hash = NULL;

    GtkTreeIter interface_iter_a;
    GtkTreeIter interface_iter_b;
    GtkTreeIter sensor_iter_a;
    GtkTreeIter sensor_iter_b;
    gboolean found_a = FALSE;
    gboolean found_b = FALSE;

    /* iterate through the sensor tree
     * code from sensors-applet.c sensors_applet_add_sensor()
     * first go through the interfaces */
    for (not_end_of_interfaces = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter);
        not_end_of_interfaces;
        not_end_of_interfaces = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter)) {

        /* get interface name */
        gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter, INTERFACE_COLUMN, &interface_name, -1);

        /* then go through the individual sensors under one interface */
        for (not_end_of_sensors = gtk_tree_model_iter_children(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter,  &interfaces_iter);
            not_end_of_sensors;
            not_end_of_sensors = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter)) {

            gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter,
                       PATH_COLUMN, &sensor_path,
                       ID_COLUMN, &sensor_id,
                       -1);

            sensor_hash = sensors_applet_settings_get_unique_id (interface_name, sensor_id, sensor_path);

            /* save interface_name and iters for swap */
            if (g_ascii_strcasecmp(sensor_hash, hash_a) == 0) {
                /* can copy by value, don't free */
                interface_iter_a = interfaces_iter;
                sensor_iter_a = sensors_iter;
                found_a = TRUE;
            }

            if (g_ascii_strcasecmp(sensor_hash, hash_b) == 0) {
                interface_iter_b = interfaces_iter;
                sensor_iter_b = sensors_iter;
                found_b = TRUE;
            }

            g_free(sensor_id);
            g_free(sensor_path);
            g_free(sensor_hash);

            /* break after freeing loop variables */
            if (found_a && found_b) {
                break;
            }
        }

        g_free(interface_name);

        /* break after freeing loop variables */
        if (found_a && found_b) {
            break;
        }
    }

    /* make the switch */
    if (found_a && found_b) {

        GtkTreeIter first_iter;
        GtkTreeIter iter_next;

        gint ret_val;

        /* set a's interface to be the first interface in the sensors tree */
        if (a_is_first) {

            /* this 'fails' if the tree is empty, but this is already checked with the first loop */
            gtk_tree_model_get_iter_first (GTK_TREE_MODEL(sensors_applet->sensors), &first_iter);

            /* we are only interested in the case, where the two are not equal
             * that means the a's interface is not the first interface */
            if ( 0 != sensors_applet_settings_sort_sensors_iter_compare (sensors_applet, first_iter, interface_iter_a)) {

                /* it should be, so it needs to be moved to the first position */
                gtk_tree_store_move_after (GTK_TREE_STORE(sensors_applet->sensors), &interface_iter_a, NULL);
            }

        }

        /* check b's interface position
         * (1) if it is before a's if. - not possible, bc I have just set a's if. as first
         * afterwards every new if. will be set right after (one) a's if.
         * (0) if it is the same as a's, noop
         * (-1) if it is after a, check if it is right after a, if not move it there */
        ret_val = sensors_applet_settings_sort_sensors_iter_compare (sensors_applet, interface_iter_a, interface_iter_b);
        if (-1 == ret_val) {

            /* set iter_next to the iter after a's if. iter, can copy like this */
            iter_next = interface_iter_a;
            /* this 'fails' if there is no node after a's if. iter, but we already know, that at least b's if. iter is */
            gtk_tree_model_iter_next (GTK_TREE_MODEL(sensors_applet->sensors), &iter_next);

            /* the node right after a's if. iter is not b's if. iter, so move b's if. iter */
            if (0 != sensors_applet_settings_sort_sensors_iter_compare (sensors_applet, iter_next, interface_iter_b)) {

                gtk_tree_store_move_after (GTK_TREE_STORE(sensors_applet->sensors), &interface_iter_b, &interface_iter_a);
            }
        }

        /* at this point the interfaces are sorted, the sensors are next */
        /* set a to be the first sensor in the sensors tree, under the first if. iter */
        if (a_is_first) {

            /* this 'fails' if the tree is empty, but at least a is in it */
            gtk_tree_model_iter_children(GTK_TREE_MODEL(sensors_applet->sensors), &first_iter,  &interface_iter_a);

            /* we are only interested in the case, where the two are not equal
             * that means the a is not the first sensor */
            if ( 0 != sensors_applet_settings_sort_sensors_iter_compare (sensors_applet, first_iter, sensor_iter_a)) {

                /* it should be, so it needs to be moved to the first position */
                gtk_tree_store_move_after (GTK_TREE_STORE(sensors_applet->sensors), &sensor_iter_a, NULL);
            }

        }

        /* check b's position
         * if a's if. and b's if is the same
         * (1) if it is before a's if. - not possible, bc I have just set a as first
         * afterwards every new sensor will be set right after (one) a
         * (0) if it is the same as a's, not possible, a and b would be equal
         * (-1) if it is after a, check if it is right after a, if not move it there */
        if (0 == ret_val && -1 == sensors_applet_settings_sort_sensors_iter_compare (sensors_applet, sensor_iter_a, sensor_iter_b)) {

            /* set iter_next to the iter after a's iter, can copy like this */
            iter_next = sensor_iter_a;
            /* this 'fails' if there is no node after a's iter, but we already know, that at least b's iter is */
            gtk_tree_model_iter_next (GTK_TREE_MODEL(sensors_applet->sensors), &iter_next);

            /* the node right after a's iter is not b's iter, so move b's iter */
            if (0 != sensors_applet_settings_sort_sensors_iter_compare (sensors_applet, iter_next, sensor_iter_b)) {

                gtk_tree_store_move_after (GTK_TREE_STORE(sensors_applet->sensors), &sensor_iter_b, &sensor_iter_a);
            }
        }

        /* if a's if. and b's if is not the same
         * as b comes after a, b must be under a new if.
         * and so b must be the first node under that if. */
        /* set b to be the first sensor in the sensors tree, under the b's if. iter */
        if (-1 == ret_val) {

            /* this 'fails' if the tree is empty, but at least b is in it */
            gtk_tree_model_iter_children(GTK_TREE_MODEL(sensors_applet->sensors), &first_iter,  &interface_iter_b);

            /* we are only interested in the case, where the two are not equal
             * that means the b is not the first sensor */
            if ( 0 != sensors_applet_settings_sort_sensors_iter_compare (sensors_applet, first_iter, sensor_iter_b)) {

                /* it should be, so it needs to be moved to the first position */
                gtk_tree_store_move_after (GTK_TREE_STORE(sensors_applet->sensors), &sensor_iter_b, NULL);
            }

        }

        return 1;

    }

    /* hash_b has already been removed from the system */
    if (found_a && !found_b) {
        return 2;
    }

    /* this is the least likely case
     * if hash_a is not found, then the sensors-list starts with a sensor
     * that has already been removed from the system */
    if (!found_a) {
        return 3;
    }

    /* we should never get here */
    return 0;
}

/* sort sensors based on sensors-list array in gsettings */
gboolean sensors_applet_settings_sort_sensors (SensorsApplet *sensors_applet) {

    gchar **sensors_list = NULL;

    gchar *hash_a = NULL;
    gchar *hash_b = NULL;

    gint ret_val = -1;
    /* marks the first iteration */
    gint first_counter = 1;

    /* get sensors-list array from gsettings
     * newly allocated, has to be freed fully
     * for an empty array a pointer to a NULL pointer will be returned (g_variant_dup_strv ()) */
    sensors_list = g_settings_get_strv (sensors_applet->settings, "sensors-list");

    /* list is empty (on the first run) */
    if (NULL == *sensors_list) {
        /* still need to free the array, even if it is empty */
        g_strfreev (sensors_list);
        return FALSE;
    }

    gint i;
    /* hash lists are equal */
    gboolean hle = TRUE;
    /* compare saved sorted list to newly loaded sensors' list */
    for (i = 0; (NULL != sensors_applet->sensors_hash_array[i]) && (NULL != sensors_list[i]); i++) {

        if (g_ascii_strcasecmp(sensors_applet->sensors_hash_array[i], sensors_list[i]) != 0) {
            hle = FALSE;
            break;
        }
    }

    /* lists are the same -> nothing to do */
    if (hle) {
#ifdef SORT_DEBUG
        syslog(LOG_ERR, "sensor sort: saved list is the same as the new one, returning");
#endif

        /* still need to free the array */
        g_strfreev (sensors_list);
        return TRUE;
    }

#ifdef SORT_DEBUG
    sensors_applet_settings_print_sensors_tree (sensors_applet);
#endif

    for (i = 0; NULL != sensors_list[i]; i++) {

        /* first pass */
        if (i == 0) {
            /* make a copy */
            hash_a = g_strdup (sensors_list[i]);
            continue;
        }

        hash_b = g_strdup (sensors_list[i]);

        /* now that we have two hashes, find the two corresponding sensors and sort them
         * if i == 1, we have both a and b set, this is the first time calling this function */
        ret_val = sensors_applet_settings_sort_sensors_sort (sensors_applet, hash_a, hash_b, (i == first_counter));

        /* hash_b has already been removed from the system
         * don't free hash_a, don't reassign hash_b
         * as we already know that b is no longer in the system */
        if (2 == ret_val) {
            continue;
        }

        /* after sorting free hash_a (it should be in place) and reassign hash_b to hash_a */
        g_free (hash_a);
        hash_a = hash_b;

        /* hash_a not found
         * set hash_b as hash_a and set the first element counter
         * (essentially skip the first element in the sensors-list array) */
        if (3 == ret_val) {
            first_counter++;
        }
    }

#ifdef SORT_DEBUG
    sensors_applet_settings_print_sensors_tree (sensors_applet);
#endif

    /* hash_a already freed
     * only in the following case do we need to free it */
    if (2 == ret_val) {
        g_free (hash_a);
    }
    g_free (hash_b);

    g_strfreev (sensors_list);

    /* reorder active sensors based on reordered sensors tree */
    sensors_applet_reorder_sensors (sensors_applet);

    return TRUE;
}

/* save sensor data under a unique hash
 * save sensor sort in an array, with above hash */
gboolean sensors_applet_settings_save_sensors (SensorsApplet *sensors_applet) {

    /* write everything to GSettings except VISIBLE and ALARM_TIMEOUT_INDEX
     * for stepping through GtkTreeStore data structure */
    GtkTreeIter interfaces_iter;
    GtkTreeIter sensors_iter;
    gboolean not_end_of_interfaces = TRUE;
    gboolean not_end_of_sensors = TRUE;
    gchar *applet_path = NULL;

    gchar *current_path = NULL;
    gchar *current_id = NULL;
    gchar *current_label = NULL;
    gchar *current_interface = NULL;
    gchar *current_low_alarm_command = NULL;
    gchar *current_high_alarm_command = NULL;
    gchar *current_graph_color = NULL;
    gboolean current_enable,
             current_alarm_enable;
    gdouble current_low_value,
            current_high_value,
            current_multiplier,
            current_offset;
    guint current_alarm_timeout,
          current_sensor_type,
          current_icon_type;

    /* data structure (array) to be able to save sensors list to gsettings */
    GVariantBuilder builder;
    g_variant_builder_init (&builder, G_VARIANT_TYPE (GSGVTSA));

    applet_path = mate_panel_applet_get_preferences_path (sensors_applet->applet);

    /* now step through the GtkTreeStore sensors to find which sensors are available / loaded */
    for (not_end_of_interfaces = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter);
        not_end_of_interfaces;
        not_end_of_interfaces = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &interfaces_iter)) {

        for (not_end_of_sensors = gtk_tree_model_iter_children(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter, &interfaces_iter);
            not_end_of_sensors;
            not_end_of_sensors = gtk_tree_model_iter_next(GTK_TREE_MODEL(sensors_applet->sensors), &sensors_iter)) {

            gtk_tree_model_get(GTK_TREE_MODEL(sensors_applet->sensors),
                               &sensors_iter,
                               PATH_COLUMN, &current_path,
                               ID_COLUMN, &current_id,
                               LABEL_COLUMN, &current_label,
                               INTERFACE_COLUMN, &current_interface,
                               SENSOR_TYPE_COLUMN, &current_sensor_type,
                               ENABLE_COLUMN, &current_enable,
                               LOW_VALUE_COLUMN, &current_low_value,
                               HIGH_VALUE_COLUMN, &current_high_value,
                               ALARM_ENABLE_COLUMN, &current_alarm_enable,
                               LOW_ALARM_COMMAND_COLUMN, &current_low_alarm_command,
                               HIGH_ALARM_COMMAND_COLUMN, &current_high_alarm_command,
                               ALARM_TIMEOUT_COLUMN, &current_alarm_timeout,
                               MULTIPLIER_COLUMN, &current_multiplier,
                               OFFSET_COLUMN, &current_offset,
                               ICON_TYPE_COLUMN, &current_icon_type,
                               GRAPH_COLOR_COLUMN, &current_graph_color,
                               -1);

            /* GSettings unique id for one sensor data */
            gchar *gsuid = sensors_applet_settings_get_unique_id (current_interface, current_id, current_path);

            /* save sensor uid to gvariant array */
            g_variant_builder_add(&builder,
                GSGVTS,       /* must be related to the G_VARIANT_TYPE in init and gsettings schema */
                gsuid);

            /* save sensor data to gsettings individually
             * g_strdup_printf doesn't free args!
             * applet_path is freed at the end */
            gchar *path = g_strdup_printf ("%s%s/", applet_path, gsuid);
            g_free (gsuid);

            GSettings *settings;
            settings = g_settings_new_with_path ("org.mate.sensors-applet.sensor", path);
            g_free (path);

            /* wait until g_settings_apply() is called to save all changes at once to gsettings */
            g_settings_delay (settings);
            g_settings_set_string (settings, PATH, current_path);
            g_settings_set_string (settings, ID, current_id);
            g_settings_set_string (settings, LABEL, current_label);
            g_settings_set_string (settings, INTERFACE, current_interface);
            g_settings_set_int (settings, SENSOR_TYPE, current_sensor_type);
            g_settings_set_boolean (settings, ENABLED, current_enable);
            g_settings_set_double (settings, LOW_VALUE, current_low_value);
            g_settings_set_double (settings, HIGH_VALUE, current_high_value);
            g_settings_set_boolean (settings, ALARM_ENABLED, current_alarm_enable);
            g_settings_set_string (settings, LOW_ALARM_COMMAND, current_low_alarm_command);
            g_settings_set_string (settings, HIGH_ALARM_COMMAND, current_high_alarm_command);
            g_settings_set_int (settings, ALARM_TIMEOUT, current_alarm_timeout);
            g_settings_set_double (settings, MULTIPLIER, current_multiplier);
            g_settings_set_double (settings, OFFSET, current_offset);
            g_settings_set_int (settings, ICON_TYPE, current_icon_type);
            g_settings_set_string (settings, GRAPH_COLOR, current_graph_color);
            g_settings_apply (settings);
            g_object_unref (settings);

        }
    }

    /* save the sensor-list array to gsettings
     * builder is freed by g_variant_builder_end()
     * the gvariant returned from g_variant_builder_end() is floating, so it is freed by g_settings_set_value() */
    g_settings_set_value (sensors_applet->settings, "sensors-list", g_variant_builder_end (&builder));

    g_free (applet_path);

    return TRUE;
}

