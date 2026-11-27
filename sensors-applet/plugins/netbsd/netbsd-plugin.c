/*
 * Copyright (C) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#include <fcntl.h>
#include <unistd.h>
#include <prop/proplib.h>
#include <sys/envsys.h>

#include "netbsd-plugin.h"

static int sysmon_fd = -1;

static gdouble netbsd_plugin_get_sensor_value(const gchar *path,
					      const gchar *id,
					      SensorType type,
					      GError **error) {

	prop_dictionary_t dict;
	prop_array_t array;
	prop_object_iterator_t iter, siter;
	prop_object_t obj, sobj;
	const gchar *desc = NULL, *state = NULL;
	gdouble sensor_val = -1.0f;
	int64_t val;

	if (prop_dictionary_recv_ioctl(sysmon_fd, ENVSYS_GETDICTIONARY, &dict) == -1)
		return sensor_val;

	iter = prop_dictionary_iterator(dict);
	while ((obj = prop_object_iterator_next(iter)) != NULL) {
		if (strcmp(path, prop_dictionary_keysym_cstring_nocopy(obj)) != 0)
			continue;

		array = prop_dictionary_get_keysym(dict, obj);
		siter = prop_array_iterator(array);
		while ((sobj = prop_object_iterator_next(siter)) != NULL) {
			if (prop_dictionary_get_cstring_nocopy(sobj, "description", &desc) == false)
				continue;
			if (strcmp(id, desc) != 0)
				continue;

			prop_dictionary_get_cstring_nocopy(sobj, "state", &state);
			prop_dictionary_get_int64(sobj, "cur-value", &val);

			if (strcmp(state, "valid") == 0) {
				switch (type) {
				case TEMP_SENSOR:
					/* K to C */
					sensor_val = ((val - 273150000) / 1000000.0);
					break;
				case FAN_SENSOR:
					/* RPM */
					sensor_val = (gdouble)val;
					break;
				case VOLTAGE_SENSOR:
					/* uV to V */
					sensor_val = val / 1000000.0;
					break;
				default:
					break;
				}
			}
			break;
		}
		prop_object_iterator_release(siter);
		break;
	}
	prop_object_iterator_release(iter);

	prop_object_release(dict);

	return sensor_val;
}

static IconType netbsd_plugin_get_sensor_icon(const gchar *desc) {
	IconType icontype = GENERIC_ICON;

	if (strcasestr(desc, "cpu") != NULL)
		icontype = CPU_ICON;
	else if (strcasestr(desc, "gpu") != NULL)
		icontype = GPU_ICON;
	else if (strcasestr(desc, "batt") != NULL)
		icontype = BATTERY_ICON;
	else if (strcasestr(desc, "mem") != NULL)
		icontype = MEMORY_ICON;
	else if (strcasestr(desc, "disk") != NULL || strcasestr(desc, "hdd") != NULL)
		icontype = HDD_ICON;

	return icontype;
}

static void netbsd_plugin_get_sensors(GList **sensors) {
	prop_dictionary_t dict;
	prop_array_t array;
	prop_object_iterator_t iter, siter;
	prop_object_t obj, sobj;

	sysmon_fd = open("/dev/sysmon", O_RDONLY);
	if (sysmon_fd == -1)
		return;

	if (prop_dictionary_recv_ioctl(sysmon_fd, ENVSYS_GETDICTIONARY, &dict) == -1) {
		close(sysmon_fd);
		return;
	}

	iter = prop_dictionary_iterator(dict);
	while ((obj = prop_object_iterator_next(iter)) != NULL) {
		const gchar *path = prop_dictionary_keysym_cstring_nocopy(obj);
		array = prop_dictionary_get_keysym(dict, obj);
		siter = prop_array_iterator(array);
		while ((sobj = prop_object_iterator_next(siter)) != NULL) {
			const gchar *type = NULL, *desc = NULL;
			if (prop_dictionary_get_cstring_nocopy(sobj, "type", &type) == false)
				continue;
			if (prop_dictionary_get_cstring_nocopy(sobj, "description", &desc) == false)
				continue;

			SensorType sensortype;
			if (strcmp(type, "Fan") == 0)
				sensortype = FAN_SENSOR;
			else if (strcmp(type, "Temperature") == 0)
				sensortype = TEMP_SENSOR;
			else if (strcmp(type, "Voltage AC") == 0 || strcmp(type, "Voltage DC") == 0)
				sensortype = VOLTAGE_SENSOR;
			else
				continue;

			IconType icontype = netbsd_plugin_get_sensor_icon(desc);

			sensors_applet_plugin_add_sensor(sensors,
							 g_strdup(path),
							 g_strdup(desc),
							 g_strdup(desc),
							 sensortype,
							 TRUE,
							 icontype,
							 DEFAULT_GRAPH_COLOR);
		}
		prop_object_iterator_release(siter);
	}
	prop_object_iterator_release(iter);

	prop_object_release(dict);
}

static GList *netbsd_plugin_init(void) {
	GList *sensors = NULL;

	netbsd_plugin_get_sensors(&sensors);

	return sensors;
}

/* API functions */
const gchar *sensors_applet_plugin_name(void) {
	return "netbsd";
}

GList *sensors_applet_plugin_init(void) {
	return netbsd_plugin_init();
}

gdouble sensors_applet_plugin_get_sensor_value(const gchar *path,
					       const gchar *id,
					       SensorType type,
					       GError **error) {

	return netbsd_plugin_get_sensor_value(path, id, type, error);
}
