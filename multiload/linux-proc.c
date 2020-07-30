/* From wmload.c, v0.9.2, licensed under the GPL. */
#include <config.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <glibtop/loadavg.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>
#include <glibtop/mountlist.h>
#include <glibtop/fsusage.h>

#include "linux-proc.h"
#include "autoscaler.h"

static const unsigned needed_cpu_flags =
(1 << GLIBTOP_CPU_USER) +
(1 << GLIBTOP_CPU_IDLE) +
(1 << GLIBTOP_CPU_SYS) +
(1 << GLIBTOP_CPU_NICE);

#if 0
static const unsigned needed_page_flags =
(1 << GLIBTOP_SWAP_PAGEIN) +
(1 << GLIBTOP_SWAP_PAGEOUT);
#endif

static const unsigned needed_mem_flags =
(1 << GLIBTOP_MEM_USED) +
(1 << GLIBTOP_MEM_FREE);

static const unsigned needed_swap_flags =
(1 << GLIBTOP_SWAP_USED) +
(1 << GLIBTOP_SWAP_FREE);

static const unsigned needed_loadavg_flags =
(1 << GLIBTOP_LOADAVG_LOADAVG);

static const unsigned needed_netload_flags =
(1 << GLIBTOP_NETLOAD_IF_FLAGS) +
(1 << GLIBTOP_NETLOAD_BYTES_TOTAL);

void
GetLoad (int        Maximum,
         int        data [cpuload_n],
         LoadGraph *g)
{
    MultiloadApplet *multiload;
    glibtop_cpu cpu;
    long cpu_aux [cpuload_n], used = 0, total = 0;
    int current_scaled, used_scaled = 0;
    int i;

    glibtop_get_cpu (&cpu);

    g_return_if_fail ((cpu.flags & needed_cpu_flags) == needed_cpu_flags);

    multiload = g->multiload;

    multiload->cpu_time [cpuload_usr]    = cpu.user;
    multiload->cpu_time [cpuload_nice]   = cpu.nice;
    multiload->cpu_time [cpuload_sys]    = cpu.sys;
    multiload->cpu_time [cpuload_iowait] = cpu.iowait + cpu.irq + cpu.softirq;
    multiload->cpu_time [cpuload_free]   = cpu.idle;

    if (!multiload->cpu_initialized) {
        memcpy (multiload->cpu_last, multiload->cpu_time, sizeof (multiload->cpu_last));
        multiload->cpu_initialized = 1;
    }

    for (i = 0; i < cpuload_n; i++) {
        cpu_aux [i] = multiload->cpu_time [i] - multiload->cpu_last [i];
        total += cpu_aux [i];
    }

    for (i = 0; i < cpuload_free; i++) {
        used += cpu_aux [i];
        current_scaled = rint ((float)(cpu_aux [i] * Maximum) / (float)total);
        used_scaled += current_scaled;
        data [i] = current_scaled;
    }
    data [cpuload_free] = Maximum - used_scaled;

    multiload->cpu_used_ratio = (float)(used) / (float)total;

    memcpy (multiload->cpu_last, multiload->cpu_time, sizeof multiload->cpu_last);
}

void
GetDiskLoad (int        Maximum,
             int        data [diskload_n],
             LoadGraph *g)
{
    static gboolean first_call = TRUE;
    static guint64 lastread = 0, lastwrite = 0;
    static AutoScaler scaler;

    guint i;
    int max;
    gboolean nvme_diskstats;

    guint64 read, write;
    guint64 readdiff, writediff;

    MultiloadApplet *multiload;

    multiload = g->multiload;

    nvme_diskstats = g_settings_get_boolean (multiload->settings, "diskload-nvme-diskstats");

    if(first_call)
    {
        autoscaler_init(&scaler, 60, 500);
    }

    read = write = 0;

    if (nvme_diskstats)
    {
        FILE *fdr;
        char line[255];
        guint64 s_read, s_write;

        fdr = fopen("/proc/diskstats", "r");
        if (!fdr)
        {
            g_settings_set_boolean (multiload->settings, "diskload-nvme-diskstats", FALSE);
            return;
        }

        while (fgets(line, 255, fdr))
        {
            /* Match main device, rather than individual partitions (e.g. nvme0n1) */
            if (!g_regex_match_simple("\\snvme\\d+\\w+\\d+\\s", line, 0, 0))
            {
                continue;
            }

            /*
               6 - sectors read
               10 - sectors written
               */
            if (sscanf(line, "%*d %*d %*s %*d %*d %ld %*d %*d %*d %ld", &s_read, &s_write) == 2)
            {
                read += 512 * s_read;
                write += 512 * s_write;
            }
        }
        fclose(fdr);
    }
    else
    {
        glibtop_mountlist mountlist;
        glibtop_mountentry *mountentries;

        mountentries = glibtop_get_mountlist (&mountlist, FALSE);

        for (i = 0; i < mountlist.number; i++)
        {
            struct statvfs statresult;
            glibtop_fsusage fsusage;

            if (strstr (mountentries[i].devname, "/dev/") == NULL)
                continue;

            if (strstr (mountentries[i].mountdir, "/media/") != NULL)
                continue;

            if (statvfs (mountentries[i].mountdir, &statresult) < 0)
            {
                g_debug ("Failed to get statistics for mount entry: %s. Reason: %s. Skipping entry.",
                         mountentries[i].mountdir, strerror(errno));
                continue;
            }

            glibtop_get_fsusage(&fsusage, mountentries[i].mountdir);
            read += fsusage.read; write += fsusage.write;
        }

        g_free(mountentries);
    }

    readdiff  = read - lastread;
    writediff = write - lastwrite;

    lastread  = read;
    lastwrite = write;

    if(first_call)
    {
        first_call = FALSE;
        memset(data, 0, 3 * sizeof data[0]);
        return;
    }

    max = autoscaler_get_max(&scaler, readdiff + writediff);

    multiload->diskload_used_ratio = (float)(readdiff + writediff) / (float)max;

    data [diskload_read]  = rint ((float)Maximum *  (float)readdiff / (float)max);
    data [diskload_write] = rint ((float)Maximum * (float)writediff / (float)max);
    data [diskload_free]  = Maximum - (data [0] + data[1]);
}

#if 0
void
GetPage (int Maximum, int data [3], LoadGraph *g)
{
    static int max = 100; /* guess at maximum page rate (= in + out) */
    static u_int64_t lastin = 0;
    static u_int64_t lastout = 0;
    int in, out, idle;

    glibtop_swap swap;

    glibtop_get_swap (&swap);

    assert ((swap.flags & needed_page_flags) == needed_page_flags);

    if ((lastin > 0) && (lastin < swap.pagein)) {
	in = swap.pagein - lastin;
    }
    else {
	in = 0;
    }
    lastin = swap.pagein;

    if ((lastout > 0) && (lastout < swap.pageout)) {
	out = swap.pageout - lastout;
    }
    else {
	out = 0;
    }
    lastout = swap.pageout;

    if ((in + out) > max) {
	/* Maximum page rate has increased. Change the scale without
	   any indication whatsoever to the user (not a nice thing to
	   do). */
	max = in + out;
    }

    in   = rint (Maximum * ((float)in / max));
    out  = rint (Maximum * ((float)out / max));
    idle = Maximum - in - out;

    data [0] = in;
    data [1] = out;
    data [2] = idle;
}
#endif /* 0 */

/* GNU/Linux: The Shared memory (mem.shared) is part of the Cached memory
 * (mem.cached).
 *   aux [memload_user]   = (mem.total - mem.free) - (mem.cached + mem.buffer)
 *   aux [memload_buffer] = mem.buffer;
 *   aux [memload_cached] = mem.cached;
 *
 * Other operating systems:
 *   aux [memload_user]   = mem.user;
 *   aux [memload_shared] = mem.shared;
 *   aux [memload_buffer] = mem.buffer;
 *   aux [memload_cached] = mem.cached;
 */
void
GetMemory (int        Maximum,
           int        data [memload_n],
           LoadGraph *g)
{
    MultiloadApplet *multiload;
    glibtop_mem mem;
    guint64 aux [memload_n], cache = 0;
    int current_scaled, used_scaled = 0;
    int i;

    glibtop_get_mem (&mem);

    g_return_if_fail ((mem.flags & needed_mem_flags) == needed_mem_flags);

#ifndef __linux__
    aux [memload_user]   = mem.user;
    aux [memload_shared] = mem.shared;
#else
    aux [memload_user]   = mem.total - mem.free - mem.buffer - mem.cached;
#endif /* __linux__ */
    aux [memload_buffer] = mem.buffer;
    aux [memload_cached] = mem.cached;

    for (i = 0; i < memload_free; i++) {
        current_scaled = rint ((float)(aux [i] * Maximum) / (float)mem.total);
        if (i != memload_user) {
            cache += aux [i];
        }
        used_scaled += current_scaled;
        data [i] = current_scaled;
    }
    data [memload_free] = MAX (Maximum - used_scaled, 0);

    multiload = g->multiload;
    multiload->memload_user  = aux [memload_user];
    multiload->memload_cache = cache;
    multiload->memload_total = mem.total;
}

void
GetSwap (int        Maximum,
         int        data [swapload_n],
         LoadGraph *g)
{
    int used;
    MultiloadApplet *multiload;
    glibtop_swap swap;

    glibtop_get_swap (&swap);
    g_return_if_fail ((swap.flags & needed_swap_flags) == needed_swap_flags);

    multiload = g->multiload;

    if (swap.total == 0) {
        used = 0;
        multiload->swapload_used_ratio = 0.0f;
    }
    else {
        float ratio;

        ratio = (float)swap.used / (float)swap.total;
        used = rint ((float) Maximum * ratio);
        multiload->swapload_used_ratio = ratio;
    }

    data [0] = used;
    data [1] = Maximum - used;
}

void
GetLoadAvg (int       Maximum,
            int        data [2],
            LoadGraph *g)
{
    glibtop_loadavg loadavg;
    MultiloadApplet *multiload;

    glibtop_get_loadavg (&loadavg);

    g_return_if_fail ((loadavg.flags & needed_loadavg_flags) == needed_loadavg_flags);

    multiload = g->multiload;
    multiload->loadavg1 = loadavg.loadavg[0];

    data [0] = rint ((float) Maximum * loadavg.loadavg[0]);
    data [1] = Maximum - data[0];
}

/*
 * Return true if a network device (identified by its name) is virtual
 * (ie: not corresponding to a physical device). In case it is a physical
 * device or unknown, returns false.
 */
static gboolean
is_net_device_virtual(char *device)
{
    /*
     * There is not definitive way to find out. On some systems (Linux
     * kernels ≳ 2.19 without option SYSFS_DEPRECATED), there exist a
     * directory /sys/devices/virtual/net which only contains virtual
     * devices.  It's also possible to detect by the fact that virtual
     * devices do not have a symlink "device" in
     * /sys/class/net/name-of-dev/ .  This second method is more complex
     * but more reliable.
     */
    gboolean ret = FALSE;
    char *path = malloc (strlen (device) + strlen ("/sys/class/net//device") + 1);

    if (path == NULL)
        return FALSE;

    /* Check if /sys/class/net/name-of-dev/ exists (may be old linux kernel
     * or not linux at all). */
    do {
        if (sprintf(path, "/sys/class/net/%s", device) < 0)
            break;
        if (access(path, F_OK) != 0)
            break; /* unknown */

        if (sprintf(path, "/sys/class/net/%s/device", device) < 0)
            break;
        if (access(path, F_OK) != 0)
            ret = TRUE;
    } while (0);

    free (path);
    return ret;
}

void
GetNet (int        Maximum,
        int        data [4],
        LoadGraph *g)
{
    enum Types {
        IN_COUNT = 0,
        OUT_COUNT = 1,
        LOCAL_COUNT = 2,
        COUNT_TYPES = 3
    };

    static int ticks = 0;
    static gulong past[COUNT_TYPES] = {0};
    static AutoScaler scaler;

    gulong present[COUNT_TYPES] = {0};

    guint i;
    gchar **devices;
    glibtop_netlist netlist;

    MultiloadApplet *multiload;

    multiload = g->multiload;

    if(ticks == 0)
    {
        autoscaler_init(&scaler, 60, 501);
    }

    devices = glibtop_get_netlist(&netlist);

    for(i = 0; i < netlist.number; ++i)
    {
        glibtop_netload netload;

        glibtop_get_netload(&netload, devices[i]);

        g_return_if_fail((netload.flags & needed_netload_flags) == needed_netload_flags);

        if (!(netload.if_flags & (1L << GLIBTOP_IF_FLAGS_UP)))
            continue;

        if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_LOOPBACK)) {
            /* for loopback in and out are identical, so only count in */
            present[LOCAL_COUNT] += netload.bytes_in;
            continue;
        }

        /*
         * Do not include virtual devices (VPN, PPPOE...) to avoid
         * counting the same throughput several times.
         */
        if (is_net_device_virtual(devices[i]))
            continue;

        present[IN_COUNT] += netload.bytes_in;
        present[OUT_COUNT] += netload.bytes_out;
    }

    g_strfreev(devices);
    netspeed_add (multiload->netspeed_in, present[IN_COUNT]);
    netspeed_add (multiload->netspeed_out, present[OUT_COUNT]);

    if(ticks < 2) /* avoid initial spike */
    {
        ticks++;
        memset(data, 0, COUNT_TYPES * sizeof data[0]);
    }
    else
    {
        int delta[COUNT_TYPES];

        for (i = 0; i < COUNT_TYPES; i++)
        {
            /* protect against weirdness */
            if (present[i] >= past[i])
                delta[i] = (present[i] - past[i]);
            else
                delta[i] = 0;
        }

        for (i = 0; i < COUNT_TYPES; i++)
            data[i]   = delta[i];

    }

    memcpy(past, present, sizeof past);
}
