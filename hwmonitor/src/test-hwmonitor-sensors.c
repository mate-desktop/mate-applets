#include <glib.h>

#include "hwmonitor-sensors.h"

static void
test_cpu_parser (void)
{
    guint64 total = 0, idle = 0;
    g_assert_true (hwmonitor_parse_cpu_line ("cpu  100 20 30 40 5 0 0 0", &total, &idle));
    g_assert_cmpuint (total, ==, 195);
    g_assert_cmpuint (idle, ==, 45);
    g_assert_false (hwmonitor_parse_cpu_line ("not-cpu 1 2", &total, &idle));
}

static void
test_memory_parser (void)
{
    guint64 available = 0;
    const gchar *meminfo = "MemTotal:       100000 kB\nMemFree:        20000 kB\nMemAvailable:   54321 kB\n";
    g_assert_true (hwmonitor_parse_mem_available (meminfo, &available));
    g_assert_cmpuint (available, ==, 54321);
    g_assert_false (hwmonitor_parse_mem_available ("MemTotal: 1 kB\n", &available));
}

static void
test_temperature_parser (void)
{
    gdouble temperature = 0.0;
    g_assert_true (hwmonitor_parse_millidegrees ("67000\n", &temperature));
    g_assert_cmpfloat_with_epsilon (temperature, 67.0, 0.001);
    g_assert_false (hwmonitor_parse_millidegrees ("not-a-number", &temperature));
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);
    g_test_add_func ("/hwmonitor/cpu-parser", test_cpu_parser);
    g_test_add_func ("/hwmonitor/memory-parser", test_memory_parser);
    g_test_add_func ("/hwmonitor/temperature-parser", test_temperature_parser);
    return g_test_run ();
}
