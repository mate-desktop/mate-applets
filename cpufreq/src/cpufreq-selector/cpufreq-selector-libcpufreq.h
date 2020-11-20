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
 * Authors : Carlos Garc�a Campos <carlosgc@gnome.org>
 */

#ifndef __CPUFREQ_SELECTOR_LIBCPUFREQ_H__
#define __CPUFREQ_SELECTOR_LIBCPUFREQ_H__

#include <glib-object.h>

#include "cpufreq-selector.h"

G_BEGIN_DECLS

#define CPUFREQ_TYPE_SELECTOR_LIBCPUFREQ            \
    (cpufreq_selector_libcpufreq_get_type ())
#define CPUFREQ_SELECTOR_LIBCPUFREQ(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CPUFREQ_TYPE_SELECTOR_LIBCPUFREQ, CPUFreqSelectorLibcpufreq))
#define CPUFREQ_SELECTOR_LIBCPUFREQ_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST((klass), CPUFREQ_TYPE_SELECTOR_LIBCPUFREQ, CPUFreqSelectorLibcpufreqClass))
#define CPUFREQ_IS_SELECTOR_LIBCPUFREQ(obj)         \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CPUFREQ_TYPE_SELECTOR_LIBCPUFREQ))
#define CPUFREQ_IS_SELECTOR_LIBCPUFREQ_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CPUFREQ_TYPE_SELECTOR_LIBCPUFREQ))
#define CPUFREQ_SELECTOR_LIBCPUFREQ_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CPUFREQ_TYPE_SELECTOR_LIBCPUFREQ, CPUFreqSelectorLibcpufreqClass))

typedef struct _CPUFreqSelectorLibcpufreq      CPUFreqSelectorLibcpufreq;
typedef struct _CPUFreqSelectorLibcpufreqClass CPUFreqSelectorLibcpufreqClass;

struct _CPUFreqSelectorLibcpufreq {
    CPUFreqSelector parent;
};

struct _CPUFreqSelectorLibcpufreqClass {
    CPUFreqSelectorClass parent_class;
};


GType            cpufreq_selector_libcpufreq_get_type (void) G_GNUC_CONST;
CPUFreqSelector *cpufreq_selector_libcpufreq_new      (guint cpu);

G_END_DECLS

#endif /* __CPUFREQ_SELECTOR_SYSFS_H__ */
