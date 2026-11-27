#ifndef HWMonitor_SENSORS_H
#define HWMonitor_SENSORS_H

#include <glib.h>

typedef struct
{
    gboolean present;
    gboolean have_temperature;
    gboolean have_usage;
    gboolean have_vram;
    gdouble temperature_c;
    gdouble usage_percent;
    gdouble vram_free_mib;
} HwMonitorGpu;

typedef struct
{
    gboolean have_cpu_temperature;
    gboolean have_cpu_usage;
    gboolean have_memory;
    gdouble cpu_temperature_c;
    gdouble cpu_usage_percent;
    gdouble memory_available_mib;
    HwMonitorGpu gpu[2];
} HwMonitorSnapshot;

gboolean hwmonitor_parse_cpu_line (const gchar *line,
                                   guint64 *total,
                                   guint64 *idle);
gboolean hwmonitor_parse_mem_available (const gchar *contents,
                                        guint64 *available_kib);
gboolean hwmonitor_parse_millidegrees (const gchar *contents,
                                       gdouble *temperature_c);
void hwmonitor_collect (HwMonitorSnapshot *snapshot,
                        guint64 *previous_total,
                        guint64 *previous_idle);

#endif
