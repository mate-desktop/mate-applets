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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cpufreq-selector-factory.h"
#ifdef HAVE_LIBCPUFREQ
#include "cpufreq-selector-libcpufreq.h"
#else
#include "cpufreq-selector-sysfs.h"
#endif

CPUFreqSelector *
cpufreq_selector_factory_create_selector (guint cpu)
{
#ifdef HAVE_LIBCPUFREQ
    return cpufreq_selector_libcpufreq_new (cpu);
#else
    CPUFreqSelector *selector = NULL;

    if (g_file_test ("/sys/devices/system/cpu/cpu0/cpufreq", G_FILE_TEST_EXISTS)) {
        selector = cpufreq_selector_sysfs_new (cpu);
    }

    return selector;
#endif /* HAVE_LIBCPUFREQ */
}
