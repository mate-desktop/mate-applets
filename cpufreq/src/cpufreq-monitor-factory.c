/*
 * MATE CPUFreq Applet
 * Copyright (C) 2004 Carlos Garcia Campos <carlosgc@gnome.org>
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
 *
 * Authors : Carlos García Campos <carlosgc@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include "cpufreq-applet.h"
#include "cpufreq-utils.h"
#include "cpufreq-monitor-factory.h"
#ifdef HAVE_LIBCPUFREQ
#include "cpufreq-monitor-libcpufreq.h"
#else
#include "cpufreq-monitor-sysfs.h"
#include "cpufreq-monitor-cpuinfo.h"
#endif

CPUFreqMonitor *
cpufreq_monitor_factory_create_monitor (guint cpu)
{
#ifdef HAVE_LIBCPUFREQ
    return cpufreq_monitor_libcpufreq_new (cpu);
#else
    CPUFreqMonitor *monitor = NULL;

    if (g_file_test ("/sys/devices/system/cpu/cpu0/cpufreq", G_FILE_TEST_EXISTS)) {
        monitor = cpufreq_monitor_sysfs_new (cpu);
    } else if (g_file_test ("/proc/cpuinfo", G_FILE_TEST_EXISTS)) {
        /* If there is no cpufreq support it shows only the cpu frequency,
         * I think is better than do nothing. I have to notify it to the user, because
         * he could think that cpufreq is supported but it doesn't work succesfully
         */

        cpufreq_utils_display_error (_("CPU frequency scaling unsupported"),
                                     _("You will not be able to modify the frequency of your machine.  "
                                       "Your machine may be misconfigured or not have hardware support "
                                       "for CPU frequency scaling."));

        monitor = cpufreq_monitor_cpuinfo_new (cpu);
    }

    return monitor;
#endif
}

