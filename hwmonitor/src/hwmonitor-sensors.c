#include "hwmonitor-sensors.h"

#include <stdio.h>
#include <string.h>

static gchar *
read_file (const gchar *path)
{
    gchar *contents = NULL;
    gsize length;

    if (!g_file_get_contents (path, &contents, &length, NULL))
        return NULL;
    return contents;
}

static gboolean
read_double (const gchar *path, gdouble *value)
{
    gchar *contents = read_file (path);
    gchar *end = NULL;

    if (!contents)
        return FALSE;
    *value = g_ascii_strtod (contents, &end);
    g_free (contents);
    return end != NULL && end != contents;
}

gboolean
hwmonitor_parse_cpu_line (const gchar *line, guint64 *total, guint64 *idle)
{
    gchar **parts;
    guint i;
    guint numeric_index = 0;
    guint64 sum = 0;
    guint64 idle_value = 0;

    if (!line || !g_str_has_prefix (line, "cpu"))
        return FALSE;

    parts = g_strsplit_set (line, " \t\n", -1);
    for (i = 1; parts[i] != NULL; i++) {
        gchar *end = NULL;
        guint64 value;

        if (parts[i][0] == '\0')
            continue;
        value = g_ascii_strtoull (parts[i], &end, 10);
        if (end == parts[i] || *end != '\0')
            continue;
        sum += value;
        if (numeric_index == 3 || numeric_index == 4)
            idle_value += value;
        numeric_index++;
    }

    g_strfreev (parts);
    if (sum == 0)
        return FALSE;

    *total = sum;
    *idle = idle_value;
    return TRUE;
}

gboolean
hwmonitor_parse_mem_available (const gchar *contents, guint64 *available_kib)
{
    gchar **lines;
    guint i;

    if (!contents)
        return FALSE;
    lines = g_strsplit (contents, "\n", -1);
    for (i = 0; lines[i] != NULL; i++) {
        guint64 value;
        if (g_str_has_prefix (lines[i], "MemAvailable:") &&
            sscanf (lines[i] + strlen ("MemAvailable:"), "%" G_GUINT64_FORMAT, &value) == 1) {
            *available_kib = value;
            g_strfreev (lines);
            return TRUE;
        }
    }
    g_strfreev (lines);
    return FALSE;
}

gboolean
hwmonitor_parse_millidegrees (const gchar *contents, gdouble *temperature_c)
{
    gchar *end = NULL;
    gdouble value;

    if (!contents)
        return FALSE;
    value = g_ascii_strtod (contents, &end);
    if (end == contents)
        return FALSE;
    *temperature_c = value / 1000.0;
    return TRUE;
}

static gboolean
read_cpu_usage (guint64 *previous_total, guint64 *previous_idle, gdouble *usage)
{
    gchar *contents = read_file ("/proc/stat");
    gchar *line_end;
    guint64 total, idle;
    gboolean result = FALSE;

    if (!contents)
        return FALSE;
    line_end = strchr (contents, '\n');
    if (line_end)
        *line_end = '\0';
    if (hwmonitor_parse_cpu_line (contents, &total, &idle)) {
        if (*previous_total > 0 && total > *previous_total && idle >= *previous_idle) {
            guint64 total_delta = total - *previous_total;
            guint64 idle_delta = idle - *previous_idle;
            *usage = 100.0 * (1.0 - ((gdouble) idle_delta / (gdouble) total_delta));
            if (*usage < 0.0) *usage = 0.0;
            if (*usage > 100.0) *usage = 100.0;
            result = TRUE;
        }
        *previous_total = total;
        *previous_idle = idle;
    }
    g_free (contents);
    return result;
}

static gboolean
read_cpu_temperature (gdouble *temperature)
{
    GDir *dir;
    const gchar *entry;
    gdouble fallback = 0.0;
    gdouble preferred = 0.0;
    gboolean have_fallback = FALSE;
    gboolean have_preferred = FALSE;

    dir = g_dir_open ("/sys/class/hwmon", 0, NULL);
    if (!dir)
        return FALSE;
    while ((entry = g_dir_read_name (dir)) != NULL) {
        gchar *base;
        gchar *name_path;
        gchar *sensor_name;
        GDir *sensor_dir;
        const gchar *sensor_entry;
        gboolean preferred_source;

        if (!g_str_has_prefix (entry, "hwmon"))
            continue;
        base = g_build_filename ("/sys/class/hwmon", entry, NULL);
        name_path = g_build_filename (base, "name", NULL);
        sensor_name = read_file (name_path);
        preferred_source = sensor_name &&
            (g_str_has_prefix (sensor_name, "coretemp") ||
             g_str_has_prefix (sensor_name, "k10temp") ||
             g_str_has_prefix (sensor_name, "zenpower"));
        sensor_dir = g_dir_open (base, 0, NULL);
        if (sensor_dir) {
            while ((sensor_entry = g_dir_read_name (sensor_dir)) != NULL) {
                gchar *path;
                gdouble value;
                if (!g_str_has_prefix (sensor_entry, "temp") ||
                    !g_str_has_suffix (sensor_entry, "_input"))
                    continue;
                path = g_build_filename (base, sensor_entry, NULL);
                gchar *contents = read_file (path);
                if (hwmonitor_parse_millidegrees (contents, &value)) {
                    if (value > -50.0 && value < 150.0) {
                        if (preferred_source && (!have_preferred || value > preferred)) {
                            preferred = value;
                            have_preferred = TRUE;
                        } else if (!have_fallback || value > fallback) {
                            fallback = value;
                            have_fallback = TRUE;
                        }
                    }
                }
                g_free (contents);
                g_free (path);
            }
            g_dir_close (sensor_dir);
        }
        g_free (sensor_name);
        g_free (name_path);
        g_free (base);
    }
    g_dir_close (dir);
    if (have_preferred) *temperature = preferred;
    else if (have_fallback) *temperature = fallback;
    return have_preferred || have_fallback;
}

static gboolean
read_gpu_temperature (const gchar *device_path, gdouble *temperature)
{
    gchar *hwmon_path = g_build_filename (device_path, "hwmon", NULL);
    GDir *dir = g_dir_open (hwmon_path, 0, NULL);
    const gchar *entry;
    gboolean found = FALSE;

    if (!dir) {
        g_free (hwmon_path);
        return FALSE;
    }
    while (!found && (entry = g_dir_read_name (dir)) != NULL) {
        gchar *base = g_build_filename (hwmon_path, entry, NULL);
        GDir *sensor_dir = g_dir_open (base, 0, NULL);
        const gchar *sensor_entry;
        if (!sensor_dir) { g_free (base); continue; }
        while (!found && (sensor_entry = g_dir_read_name (sensor_dir)) != NULL) {
            gchar *path;
            gchar *contents;
            if (!g_str_has_prefix (sensor_entry, "temp") || !g_str_has_suffix (sensor_entry, "_input"))
                continue;
            path = g_build_filename (base, sensor_entry, NULL);
            contents = read_file (path);
            found = hwmonitor_parse_millidegrees (contents, temperature);
            g_free (contents);
            g_free (path);
        }
        g_dir_close (sensor_dir);
        g_free (base);
    }
    g_dir_close (dir);
    g_free (hwmon_path);
    return found;
}

static void
read_sysfs_gpus (HwMonitorSnapshot *snapshot)
{
    guint card;
    guint found = 0;

    for (card = 0; card < 8 && found < 2; card++) {
        gchar *device = g_strdup_printf ("/sys/class/drm/card%u/device", card);
        gchar *busy = g_build_filename (device, "gpu_busy_percent", NULL);
        gchar *vram_used = g_build_filename (device, "mem_info_vram_used", NULL);
        gchar *vram_total = g_build_filename (device, "mem_info_vram_total", NULL);
        gdouble value;
        guint64 used, total;
        HwMonitorGpu *gpu;

        if (!g_file_test (device, G_FILE_TEST_IS_DIR)) {
            g_free (vram_total); g_free (vram_used); g_free (busy); g_free (device);
            continue;
        }
        gpu = &snapshot->gpu[found++];
        gpu->present = TRUE;
        if (read_double (busy, &value)) {
            gpu->usage_percent = CLAMP (value, 0.0, 100.0);
            gpu->have_usage = TRUE;
        }
        {
            gchar *used_text = read_file (vram_used);
            gchar *total_text = read_file (vram_total);
            used = used_text ? g_ascii_strtoull (used_text, NULL, 10) : 0;
            total = total_text ? g_ascii_strtoull (total_text, NULL, 10) : 0;
            if (total >= used && total > 0) {
                gpu->vram_free_mib = (gdouble) (total - used) / (1024.0 * 1024.0);
                gpu->have_vram = TRUE;
            }
            g_free (used_text); g_free (total_text);
        }
        if (read_gpu_temperature (device, &gpu->temperature_c))
            gpu->have_temperature = TRUE;
        g_free (vram_total); g_free (vram_used); g_free (busy); g_free (device);
    }
}

static void
read_nvidia_fallback (HwMonitorSnapshot *snapshot)
{
    gchar *program = g_find_program_in_path ("nvidia-smi");
    gchar *output = NULL;
    gchar *argv[] = { program, "--query-gpu=temperature.gpu,utilization.gpu,memory.free", "--format=csv,noheader,nounits", NULL };
    gchar **lines;
    guint i;

    if (!program || !g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
                                   &output, NULL, NULL, NULL)) {
        g_free (program);
        return;
    }
    lines = g_strsplit (output, "\n", -1);
    for (i = 0; lines[i] != NULL && i < 2; i++) {
        gchar **fields = g_strsplit (lines[i], ",", 3);
        if (fields[0] && fields[1] && fields[2]) {
            HwMonitorGpu *gpu = &snapshot->gpu[i];
            gpu->present = TRUE;
            if (!gpu->have_temperature) {
                gpu->temperature_c = g_ascii_strtod (g_strstrip (fields[0]), NULL);
                gpu->have_temperature = TRUE;
            }
            if (!gpu->have_usage) {
                gpu->usage_percent = g_ascii_strtod (g_strstrip (fields[1]), NULL);
                gpu->have_usage = TRUE;
            }
            if (!gpu->have_vram) {
                gpu->vram_free_mib = g_ascii_strtod (g_strstrip (fields[2]), NULL);
                gpu->have_vram = TRUE;
            }
        }
        g_strfreev (fields);
    }
    g_strfreev (lines);
    g_free (output);
    g_free (program);
}

void
hwmonitor_collect (HwMonitorSnapshot *snapshot, guint64 *previous_total, guint64 *previous_idle)
{
    gchar *meminfo;
    guint64 available_kib;

    memset (snapshot, 0, sizeof (*snapshot));
    snapshot->have_cpu_usage = read_cpu_usage (previous_total, previous_idle, &snapshot->cpu_usage_percent);
    snapshot->have_cpu_temperature = read_cpu_temperature (&snapshot->cpu_temperature_c);
    meminfo = read_file ("/proc/meminfo");
    if (hwmonitor_parse_mem_available (meminfo, &available_kib)) {
        snapshot->memory_available_mib = (gdouble) available_kib / 1024.0;
        snapshot->have_memory = TRUE;
    }
    g_free (meminfo);
    read_sysfs_gpus (snapshot);
    read_nvidia_fallback (snapshot);
}
