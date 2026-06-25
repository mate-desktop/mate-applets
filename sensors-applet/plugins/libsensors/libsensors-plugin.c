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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SENSORS_SENSORS_H
#include <sensors/sensors.h>
#endif

#include "libsensors-plugin.h"

const gchar *plugin_name = "libsensors";

GHashTable *hash_table = NULL;

enum {
    LIBSENSORS_CHIP_PARSE_ERROR,
    LIBSENSORS_MISSING_FEATURE_ERROR,
    LIBSENSORS_REGEX_URL_COMPILE_ERROR,
    LIBSENSORS_CHIP_NOT_FOUND_ERROR
};

#if SENSORS_API_VERSION < 0x400
#define LIBSENSORS_CONFIG_FILE "/etc/sensors.conf"
#define LIBSENSORS_ALTERNATIVE_CONFIG_FILE "/usr/local/etc/sensors.conf"
#endif

GRegex *uri_re;

static char *get_chip_name_string(const sensors_chip_name *chip) {
    char *name;

#if SENSORS_API_VERSION < 0x400
    /* taken from lm-sensors:prog/sensors/main.c:sprintf_chip_name */
    switch (chip->bus) {
        case SENSORS_CHIP_NAME_BUS_ISA:
            name = g_strdup_printf ("%s-isa-%04x", chip->prefix, chip->addr);
            break;
        case SENSORS_CHIP_NAME_BUS_DUMMY:
            name = g_strdup_printf ("%s-%s-%04x", chip->prefix, chip->busname, chip->addr);
            break;
            case SENSORS_CHIP_NAME_BUS_PCI:
                    name = g_strdup_printf ("%s-pci-%04x", chip->prefix, chip->addr);
                    break;
        default:
            name = g_strdup_printf ("%s-i2c-%d-%02x", chip->prefix, chip->bus, chip->addr);
            break;
    }

#else
/* adapted from lm-sensors:prog/sensors/main.c:sprintf_chip_name in lm-sensors-3.0 */
#define BUF_SIZE 200
    static char buf[BUF_SIZE];
    if (sensors_snprintf_chip_name(buf, BUF_SIZE, chip) < 0) {
        name = NULL;
    } else {
        name = g_strdup (buf);
    }
#endif

    return name;
}

#if SENSORS_API_VERSION < 0x400
static SensorType get_sensor_type (const char *name) {
    SensorType type = CURRENT_SENSOR;

    if (g_strrstr(name, "in")) {
        type = VOLTAGE_SENSOR;
    }
    else if (g_strrstr(name, "fan")) {
        type = FAN_SENSOR;
    }
    else if (g_strrstr(name, "temp")) {
        type = TEMP_SENSOR;
    } else {
        g_debug("sensor %s not recognised as either voltage, fan or temp sensor - assuming is a current sensor", name);
        type = CURRENT_SENSOR;
    }
    return type;
}

/* If a sensor is 'interesting' to us then return its label, otherwise NULL. */
static char *get_sensor_interesting_label (sensors_chip_name chip, int feature) {
    char *label;

    if (sensors_get_ignored(chip, feature) && (sensors_get_label(chip, feature, &label) == 0)) {
        if (! (strcmp ("alarms", label) == 0 || strncmp ("sensor", label, 6) == 0)) {
            return label;
        } else {
            free (label);
        }
    }

    return NULL;
}

static const sensors_feature_data * get_sensor_min_max(const sensors_chip_name *chip,
                                                        int *n1, int *n2,
                                                        int number,
                                                        gdouble *low_value,
                                                        gdouble *high_value) {

    const sensors_feature_data *data;
    double value;

    /* The sub features are returned directly after the main feature by
       sensors_get_all_features(), so no need to iterate over all features */
    while ((data = sensors_get_all_features (*chip, n1, n2)) != NULL && data->mapping == number) {
        if ((data->mode & SENSORS_MODE_R) && (sensors_get_feature(*chip, data->number, &value) == 0) && data->name != NULL) {

            if (g_str_has_suffix(data->name, "_min")) {
                *low_value = value;
                g_debug("overriding low value of sensor %s with value %f", data->name, value);
            } else if (g_str_has_suffix(data->name, "_max")) {
                *high_value = value;
                g_debug("overriding high value of sensor %s with value %f", data->name, value);
            }
        }
    }
    return data;
}

#endif

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

#if SENSORS_API_VERSION < 0x400
static void check_sensor_with_data(GList **sensors,
                                   const char * const chip_name,
                                   const sensors_chip_name *chip,
                                   int *n1, int *n2,
                                   const sensors_feature_data *data) {

    char *label;
    double value;
    SensorsAppletSensorInfo *sensor_info = NULL;

    /* ... some of which are boring ... */
    if ((label = get_sensor_interesting_label (*chip, data->number))) {
        /* ... and some of which are actually sensors */
        if ((data->mode & SENSORS_MODE_R) &&
            (data->mapping == SENSORS_NO_MAPPING) &&
            (sensors_get_feature(*chip, data->number, &value) == 0)) {    /* make sure we can actually get a value for it */

            SensorType type;
            gboolean visible;
            IconType icon;
            gdouble low_value, high_value;

            gchar *url;

            type = get_sensor_type (data->name);
            visible = (type == TEMP_SENSOR ? TRUE : FALSE);
            icon = get_sensor_icon(type);

            /* the 'path' contains all the information we need to identify this sensor later */
            url = g_strdup_printf ("sensor://%s/%d", chip_name, data->number);

            /* get low and high values */
            sensors_applet_plugin_default_sensor_limits(type, &low_value, &high_value);

            data = get_sensor_min_max(chip, n1, n2, data->number, &low_value, &high_value);
            if (data != NULL) {
                /* try adding this one */
                /* at this point we have called sensors_get_all_features() and stopped when we have a potential new sensor, so make sure we try this as well - do a recursive call */
                check_sensor_with_data(sensors, chip_name, chip, n1, n2, data);
            }

            g_hash_table_insert(hash_table, g_strdup(url), (void *)chip);
            /* the id identifies a particular sensor for the user; */
            /* we default to the label returned by libsensors */
            sensors_applet_plugin_add_sensor_with_limits(sensors,
                                                         url,
                                                         label,
                                                         label,
                                                         type,
                                                         visible,
                                                         low_value,
                                                         high_value,
                                                         icon,
                                                         DEFAULT_GRAPH_COLOR);
            g_free(url);
        }
        free (label);
    }

}

#endif

static GList *libsensors_plugin_get_sensors(void) {
    const sensors_chip_name *chip_name;
    GList *sensors = NULL;

#if SENSORS_API_VERSION < 0x400
    FILE *file;
    int i;
    g_debug("%s: using libsensors version < 4", __FUNCTION__);

    /* try to open config file, otherwise try alternate config
     * file - if neither succeed, exit */
    if ((file = fopen (LIBSENSORS_CONFIG_FILE, "r")) == NULL) {
        if ((file = fopen (LIBSENSORS_ALTERNATIVE_CONFIG_FILE, "r")) == NULL) {
            g_debug("%s: error opening libsensors config file... ", __FUNCTION__);
            return sensors;
        }
    }

    /* at this point should have an open config file, if is not
     * valid, close file and return */
    if (sensors_init(file) != 0) {
        fclose(file);
        g_debug("%s: error initing libsensors from config file...", __FUNCTION__);
        return sensors;
    }
    fclose(file);

    /* libsensors exposes a number of chips -  ... */
    i = 0;
    while ((chip_name = sensors_get_detected_chips (&i)) != NULL) {
        char *chip_name_string;
        const sensors_feature_data *data;
        int n1 = 0, n2 = 0;

        chip_name_string = get_chip_name_string(chip_name);

        /* ... each of which has one or more 'features' ... */
        while ((data = sensors_get_all_features (*chip_name, &n1, &n2)) != NULL) { /* error */
            /* fill in list for us */
            check_sensor_with_data(&sensors, chip_name_string, chip_name, &n1, &n2, data);
        }
        g_free (chip_name_string);
    }

#else
    g_debug("%s: using libsensors version >= 4", __FUNCTION__);

    int nr = 0;
    if (sensors_init(NULL) != 0) {
        g_debug("%s: error initing libsensors", __FUNCTION__);
        return sensors;
    }

    while ((chip_name = sensors_get_detected_chips(NULL, &nr))) {

        char *chip_name_string, *label;
        const sensors_subfeature *input_feature;
        const sensors_subfeature *low_feature;
        const sensors_subfeature *high_feature;
        const sensors_feature *main_feature;
        SensorType type;
        gint nr1 = 0;
        gdouble value, low, high;
        gchar *path;
        gboolean visible;
        IconType icon;

        chip_name_string = get_chip_name_string(chip_name);
        if (chip_name_string == NULL) {
            g_debug("%s: %d: error getting name string for sensor: %s\n", __FILE__, __LINE__, chip_name->path);
            continue;
        }

        while ((main_feature = sensors_get_features(chip_name, &nr1))) {
            switch (main_feature->type) {
                case SENSORS_FEATURE_IN:
                    type = VOLTAGE_SENSOR;
                    input_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_IN_INPUT);
                    low_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_IN_MIN);
                    high_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_IN_MAX);
                    break;

                case SENSORS_FEATURE_FAN:
                    type = FAN_SENSOR;
                    input_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_FAN_INPUT);
                    low_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_FAN_MIN);
                    /* no fan max feature */
                    high_feature = NULL;
                    break;

                case SENSORS_FEATURE_TEMP:
                    type = TEMP_SENSOR;
                    input_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_TEMP_INPUT);
                    low_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_TEMP_MIN);
                    high_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_TEMP_MAX);
                    if (!high_feature)
                        high_feature = sensors_get_subfeature(chip_name, main_feature, SENSORS_SUBFEATURE_TEMP_CRIT);
                    break;

                default:
                    g_debug("%s: %d: error determining type for: %s\n", __FILE__, __LINE__, chip_name_string);
                    continue;
            }

            if (!input_feature) {
                g_debug("%s: %d: could not get input subfeature for: %s\n", __FILE__, __LINE__, chip_name_string);
                continue;
            }

            /* if still here we got input feature so get label */
            label = sensors_get_label(chip_name, main_feature);
            if (!label) {
                g_debug("%s: %d: error: could not get label for: %s\n", __FILE__, __LINE__, chip_name_string);
                continue;
            }

            g_assert(chip_name_string && label);

            icon = get_sensor_icon(type);
            visible = (type == TEMP_SENSOR ? TRUE : FALSE);
            sensors_applet_plugin_default_sensor_limits(type, &low, &high);
            if (low_feature) {
                sensors_get_value(chip_name, low_feature->number, &low);
            }

            if (high_feature) {
                sensors_get_value(chip_name, high_feature->number, &high);
            }

            if (sensors_get_value(chip_name, input_feature->number, &value) < 0) {
                g_debug("%s: %d: error: could not get value for input feature of sensor: %s\n", __FILE__, __LINE__, chip_name_string);
                free(label);
                continue;
            }

            g_debug("for chip %s (type %s) got label %s and value %f", chip_name_string,
                (type == TEMP_SENSOR ? "temp" : (type == FAN_SENSOR ? "fan" : (type == VOLTAGE_SENSOR ? "voltage" : "error"))), label, value);

            path = g_strdup_printf ("sensor://%s/%d", chip_name_string, input_feature->number);
            g_hash_table_insert(hash_table, g_strdup(path), (void *)chip_name);

            sensors_applet_plugin_add_sensor_with_limits(&sensors,
                                                         path,
                                                         label,
                                                         label,
                                                         type,
                                                         visible,
                                                         low,
                                                         high,
                                                         icon,
                                                         DEFAULT_GRAPH_COLOR);
        }
        g_free(chip_name_string);

    }
#endif

    return sensors;
}

static gdouble libsensors_plugin_get_sensor_value(const gchar *path,
                                                    const gchar *id,
                                                    SensorType type,
                                                    GError **error) {
    gdouble result = 0;
    GMatchInfo *m;

    /* parse the uri into a (chip, feature) tuplet */
    g_regex_match (uri_re, path, 0, &m);
    if (g_match_info_matches (m)) {
        const sensors_chip_name *found_chip;
        int feature;

        if ((found_chip = g_hash_table_lookup(hash_table, path)) != NULL) {
            gdouble value;
            gchar *feature_str;

            feature_str = g_match_info_fetch (m, 1);
            feature = atoi(feature_str);
            g_free (feature_str);

#if SENSORS_API_VERSION < 0x400
            /* retrieve the value of the feature */
            if (sensors_get_feature (*found_chip, feature, &value) == 0) {
                result = value;
            } else {
                g_set_error (error, SENSORS_APPLET_PLUGIN_ERROR, LIBSENSORS_MISSING_FEATURE_ERROR, "Error retrieving sensor value");
            }
#else
            if (sensors_get_value(found_chip, feature, &value) >= 0) {
                result = value;
            } else {
                g_set_error (error, SENSORS_APPLET_PLUGIN_ERROR, LIBSENSORS_MISSING_FEATURE_ERROR, "Error retrieving sensor value");
            }
#endif

        } else {
            g_set_error (error, SENSORS_APPLET_PLUGIN_ERROR, LIBSENSORS_CHIP_NOT_FOUND_ERROR, "Chip not found");
        }
    } else {
        g_set_error (error, SENSORS_APPLET_PLUGIN_ERROR, LIBSENSORS_REGEX_URL_COMPILE_ERROR, "Error compiling URL regex: Not match");
    }
    g_match_info_free (m);
    return result;
}

static GList *libsensors_plugin_init(void) {
    GError *err = NULL;

    /* compile the regular expressions */
    uri_re = g_regex_new ("^sensor://[a-z0-9_-]+/([0-9]+)$", G_REGEX_CASELESS | G_REGEX_OPTIMIZE, G_REGEX_MATCH_ANCHORED, &err);
    if (err) {
        g_debug("Error compiling regexp: %s\nnot initing libsensors sensors interface", err->message);
        g_error_free (err);
        return NULL;
    }

    /* create hash table to associate path strings with sensors_chip_name
     * pointers - make sure it free's the keys strings on destroy */
    hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    return libsensors_plugin_get_sensors();
}

const gchar *sensors_applet_plugin_name(void)
{
    return plugin_name;
}

GList *sensors_applet_plugin_init(void)
{
    return libsensors_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
                                                const gchar *id,
                                                SensorType type,
                                                GError **error) {

    return libsensors_plugin_get_sensor_value(path, id, type, error);
}
