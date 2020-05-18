/*
 * MATE CPUFreq Applet
 * Copyright (C) 2008 Carlos Garcia Campos <carlosgc@gnome.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <sys/sysinfo.h>

#ifdef HAVE_POLKIT
#include <gio/gio.h>
#endif /* HAVE_POLKIT */

#include "cpufreq-selector.h"

struct _CPUFreqSelector {
	GObject parent;

#ifdef HAVE_POLKIT
	GDBusConnection *system_bus;
	GDBusProxy      *proxy;
#endif /* HAVE_POLKIT */
};

struct _CPUFreqSelectorClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE (CPUFreqSelector, cpufreq_selector, G_TYPE_OBJECT)

static void
cpufreq_selector_finalize (GObject *object)
{
	CPUFreqSelector *selector = CPUFREQ_SELECTOR (object);

#ifdef HAVE_POLKIT
	g_clear_object (&selector->proxy);
	g_clear_object (&selector->system_bus);
#endif /* HAVE_POLKIT */

	G_OBJECT_CLASS (cpufreq_selector_parent_class)->finalize (object);
}

static void
cpufreq_selector_class_init (CPUFreqSelectorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = cpufreq_selector_finalize;
}

static void
cpufreq_selector_init (CPUFreqSelector *selector)
{
}

CPUFreqSelector *
cpufreq_selector_get_default (void)
{
	static CPUFreqSelector *selector = NULL;

	if (!selector)
		selector = CPUFREQ_SELECTOR (g_object_new (CPUFREQ_TYPE_SELECTOR, NULL));

	return selector;
}

#ifdef HAVE_POLKIT
typedef enum {
	FREQUENCY,
	GOVERNOR
} CPUFreqSelectorCall;

typedef struct {
	CPUFreqSelector *selector;

	CPUFreqSelectorCall call;

	guint  cpu;
	guint  frequency;
	gchar *governor;

	guint32 parent_xid;
} SelectorAsyncData;

static void
selector_async_data_free (SelectorAsyncData *data)
{
	if (!data)
		return;

	g_free (data->governor);
	g_free (data);
}

static gboolean
cpufreq_selector_connect_to_system_bus (CPUFreqSelector *selector,
					GError         **error)
{
	if (selector->system_bus)
		return TRUE;

	selector->system_bus = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, error);

	return (selector->system_bus != NULL);
}


static gboolean
cpufreq_selector_create_proxy (CPUFreqSelector  *selector,
                               GError          **error)
{
	if (selector->proxy)
		return TRUE;

	selector->proxy = g_dbus_proxy_new_sync (selector->system_bus,
	                                         G_DBUS_PROXY_FLAGS_NONE,
	                                         NULL,
	                                         "org.mate.CPUFreqSelector",
	                                         "/org/mate/cpufreq_selector/selector",
	                                         "org.mate.CPUFreqSelector",
	                                         NULL,
	                                         error);

	return (selector->proxy != NULL);
}

static void
selector_setter_cb (GObject      *source,
                    GAsyncResult *result,
                    gpointer      user_data)
{
	GDBusProxy *proxy = G_DBUS_PROXY (source);
	SelectorAsyncData *data = (SelectorAsyncData *)user_data;
	GError *error = NULL;

	g_dbus_proxy_call_finish (proxy, result, &error);
	if (error != NULL) {
		g_warning ("%s", error->message);
		g_clear_error (&error);
	}
	selector_async_data_free (data);
}

static void
selector_set_frequency_async (SelectorAsyncData *data)
{
	GError *error = NULL;

	if (!cpufreq_selector_connect_to_system_bus (data->selector, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);

		selector_async_data_free (data);

		return;
	}

	if (!cpufreq_selector_create_proxy (data->selector, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);

		selector_async_data_free (data);
		return;
	}

	g_dbus_proxy_call (data->selector->proxy,
	                   "SetFrequency",
	                   g_variant_new ("(uu)",
	                                  data->cpu,
	                                  data->frequency),
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1,
	                   NULL,
	                   selector_setter_cb,
	                   data);
}

void
cpufreq_selector_set_frequency_async (CPUFreqSelector *selector,
				      guint            cpu,
				      guint            frequency)
{
    guint            cores;
    cores = get_nprocs() ;
	for (cpu = 0; cpu < cores; cpu = cpu+1){ 
	    SelectorAsyncData *data;

	    data = g_new0 (SelectorAsyncData, 1);

	    data->selector = selector;
	    data->call = FREQUENCY;
	    data->cpu = cpu;
	    data->frequency = frequency;

	    selector_set_frequency_async (data);
        }
}

static void
selector_set_governor_async (SelectorAsyncData *data)
{
	GError *error = NULL;

	if (!cpufreq_selector_connect_to_system_bus (data->selector, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);

		selector_async_data_free (data);

		return;
	}

	if (!cpufreq_selector_create_proxy (data->selector, &error)) {
		g_warning ("%s", error->message);
		g_error_free (error);

		selector_async_data_free (data);

		return;
	}

	g_dbus_proxy_call (data->selector->proxy,
	                   "SetGovernor",
	                   g_variant_new ("(us)",
	                                  data->cpu,
	                                  data->governor),
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1,
	                   NULL,
	                   selector_setter_cb,
	                   data);
}

void
cpufreq_selector_set_governor_async (CPUFreqSelector *selector,
				     guint            cpu,
				     const gchar     *governor)
{
    guint            cores;
    cores = get_nprocs() ;
    for (cpu = 0; cpu < cores; cpu = cpu+1){
	    SelectorAsyncData *data;

	    data = g_new0 (SelectorAsyncData, 1);

	    data->selector = selector;
	    data->call = GOVERNOR;
	    data->cpu = cpu;
	    data->governor = g_strdup (governor);

	    selector_set_governor_async (data);
   }
}
#else /* !HAVE_POLKIT */
static void
cpufreq_selector_run_command (CPUFreqSelector *selector,
			      const gchar     *args)
{
	gchar  *command;
	gchar  *path;
	GError *error = NULL;

	path = g_find_program_in_path ("cpufreq-selector");

	if (!path)
		return;

	command = g_strdup_printf ("%s %s", path, args);
	g_free (path);

	g_spawn_command_line_async (command, &error);
	g_free (command);

	if (error) {
		g_warning ("%s", error->message);
		g_error_free (error);
	}
}

void
cpufreq_selector_set_frequency_async (CPUFreqSelector *selector,
				      guint            cpu,
				      guint            frequency)
{
    guint            cores;
    cores = get_nprocs() ;

    for (cpu = 0; cpu < cores; cpu = cpu+1){
	    gchar *args;

	    args = g_strdup_printf ("-c %u -f %u", cpu, frequency);
	    cpufreq_selector_run_command (selector, args);
	    g_free (args);
    }
}

void
cpufreq_selector_set_governor_async (CPUFreqSelector *selector,
				     guint            cpu,
				     const gchar     *governor)
{
    guint            cores;
    cores = get_nprocs() ;
    for (cpu = 0; cpu < cores; cpu = cpu+1){
	    gchar *args;

	    args = g_strdup_printf ("-c %u -g %s", cpu, governor);
	    cpufreq_selector_run_command (selector, args);
	    g_free (args);
    }
}
#endif /* HAVE_POLKIT */
