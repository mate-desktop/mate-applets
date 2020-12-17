#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <mate-panel-applet.h>

G_BEGIN_DECLS

#define KEY_CPULOAD_USR_COLOR         "cpuload-color0"
#define KEY_CPULOAD_SYS_COLOR         "cpuload-color1"
#define KEY_CPULOAD_NICE_COLOR        "cpuload-color2"
#define KEY_CPULOAD_IOWAIT_COLOR      "cpuload-color3"
#define KEY_CPULOAD_IDLE_COLOR        "cpuload-color4"
#define KEY_MEMLOAD_USER_COLOR        "memload-color0"
#define KEY_MEMLOAD_SHARED_COLOR      "memload-color1"
#define KEY_MEMLOAD_BUFFER_COLOR      "memload-color2"
#define KEY_MEMLOAD_CACHED_COLOR      "memload-color3"
#define KEY_MEMLOAD_FREE_COLOR        "memload-color4"
#define KEY_NETLOAD2_IN_COLOR         "netload2-color0"
#define KEY_NETLOAD2_OUT_COLOR        "netload2-color1"
#define KEY_NETLOAD2_LOOPBACK_COLOR   "netload2-color2"
#define KEY_NETLOAD2_BACKGROUND_COLOR "netload2-color3"
#define KEY_NETLOAD2_GRIDLINE_COLOR   "netload2-color4"
#define KEY_NETLOAD2_INDICATOR_COLOR  "netload2-color5"
#define KEY_SWAPLOAD_USED_COLOR       "swapload-color0"
#define KEY_SWAPLOAD_FREE_COLOR       "swapload-color1"
#define KEY_LOADAVG_AVERAGE_COLOR     "loadavg-color0"
#define KEY_LOADAVG_BACKGROUND_COLOR  "loadavg-color1"
#define KEY_LOADAVG_GRIDLINE_COLOR    "loadavg-color2"
#define KEY_DISKLOAD_READ_COLOR       "diskload-color0"
#define KEY_DISKLOAD_WRITE_COLOR      "diskload-color1"
#define KEY_DISKLOAD_FREE_COLOR       "diskload-color2"

#define KEY_NET_THRESHOLD1 "netthreshold1"
#define KEY_NET_THRESHOLD2 "netthreshold2"
#define KEY_NET_THRESHOLD3 "netthreshold3"
#define MIN_NET_THRESHOLD1 10
#define MIN_NET_THRESHOLD2 11
#define MIN_NET_THRESHOLD3 12
#define MAX_NET_THRESHOLD1  999999998
#define MAX_NET_THRESHOLD2  999999999
#define MAX_NET_THRESHOLD3 1000000000

#define VIEW_CPULOAD_KEY   "view-cpuload"
#define VIEW_MEMLOAD_KEY   "view-memload"
#define VIEW_NETLOAD_KEY   "view-netload"
#define VIEW_SWAPLOAD_KEY  "view-swapload"
#define VIEW_LOADAVG_KEY   "view-loadavg"
#define VIEW_DISKLOAD_KEY  "view-diskload"

#define DISKLOAD_NVME_KEY  "diskload-nvme-diskstats"

#define REFRESH_RATE_KEY   "speed"
#define REFRESH_RATE_MIN   50
#define REFRESH_RATE_MAX   60000

#define GRAPH_SIZE_KEY     "size"
#define GRAPH_SIZE_MIN     10
#define GRAPH_SIZE_MAX     1000

typedef struct _MultiloadApplet MultiloadApplet;
typedef struct _LoadGraph LoadGraph;
typedef void (*LoadGraphDataFunc) (guint64, guint64 [], LoadGraph *);

#include "netspeed.h"

typedef enum {
    graph_cpuload = 0,
    graph_memload,
    graph_netload2,
    graph_swapload,
    graph_loadavg,
    graph_diskload,
    graph_n,
} E_graph;

typedef enum {
    cpuload_usr = 0,
    cpuload_sys,
    cpuload_nice,
    cpuload_iowait,
    cpuload_free,
    cpuload_n
} E_cpuload;

typedef enum {
    memload_user = 0,
    memload_shared,
    memload_buffer,
    memload_cached,
    memload_free,
    memload_n
} E_memload;

typedef enum {
    netload2_in = 0,
    netload2_out,
    netload2_loopback,
    netload2_background,
    netload2_gridline,
    netload2_indicator,
    netload2_n
} E_netload2;

typedef enum {
    swapload_used = 0,
    swapload_free,
    swapload_n
} E_swapload;

typedef enum {
    loadavg_average = 0,
    loadavg_background,
    loadavg_gridline,
    loadavg_n
} E_loadavg;

typedef enum {
    diskload_read = 0,
    diskload_write,
    diskload_free,
    diskload_n
} E_diskload;

struct _LoadGraph {
    MultiloadApplet *multiload;

    guint n;
    gint  id;
    guint speed, size;
    guint orient, pixel_size;
    gsize draw_width;
    guint64 draw_height;
    LoadGraphDataFunc get_data;

    guint allocated;

    GdkRGBA *colors;
    guint64 **data;
    guint64  *pos;

    GtkWidget *main_widget;
    GtkWidget *frame, *box, *disp;
    cairo_surface_t *surface;
    int timer_index;

    gboolean visible;
    gboolean tooltip_update;
    const gchar *name;
};

struct _MultiloadApplet
{
    MatePanelApplet *applet;

    GSettings *settings;

    LoadGraph *graphs [graph_n];

    GtkWidget *box;

    gboolean view_cpuload;
    gboolean view_memload;
    gboolean view_netload;
    gboolean view_swapload;
    gboolean view_loadavg;
    gboolean view_diskload;

    GtkWidget *about_dialog;
    GtkWidget *check_boxes [graph_n];
    GtkWidget *prop_dialog;
    GtkWidget *notebook;
    gint last_clicked;

    float    cpu_used_ratio;
    guint64  cpu_time [cpuload_n];
    guint64  cpu_last [cpuload_n];
    gboolean cpu_initialized;

    double loadavg1;

    guint64 memload_user;
    guint64 memload_cache;
    guint64 memload_total;

    float swapload_used_ratio;

    float diskload_used_ratio;
    gboolean nvme_diskstats;

    NetSpeed *netspeed_in;
    NetSpeed *netspeed_out;
    guint64 net_threshold1;
    guint64 net_threshold2;
    guint64 net_threshold3;
};

#include "load-graph.h"
#include "linux-proc.h"

/* show properties dialog */
G_GNUC_INTERNAL void
multiload_properties_cb (GtkAction       *action,
                         MultiloadApplet *ma);

/* remove the old graphs and rebuild them */
G_GNUC_INTERNAL void
multiload_applet_refresh (MultiloadApplet *ma);

/* update the tooltip to the graph's current "used" percentage */
G_GNUC_INTERNAL void
multiload_applet_tooltip_update (LoadGraph *g);

G_END_DECLS

#endif
