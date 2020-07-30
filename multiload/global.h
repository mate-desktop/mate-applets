#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <mate-panel-applet.h>

G_BEGIN_DECLS

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

typedef struct _MultiloadApplet MultiloadApplet;
typedef struct _LoadGraph LoadGraph;
typedef void (*LoadGraphDataFunc) (int, int [], LoadGraph *);

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
    memload_user = 0,
    memload_shared,
    memload_buffer,
    memload_cached,
    memload_free,
    memload_n
} E_memload;

typedef enum {
    cpuload_usr = 0,
    cpuload_sys,
    cpuload_nice,
    cpuload_iowait,
    cpuload_free,
    cpuload_n
} E_cpuload;

typedef enum {
    diskload_read = 0,
    diskload_write,
    diskload_free,
    diskload_n
} E_diskload;

typedef enum {
    swapload_used = 0,
    swapload_free,
    swapload_n
} E_swapload;

struct _LoadGraph {
    MultiloadApplet *multiload;

    guint n, id;
    guint speed, size;
    guint orient, pixel_size;
    guint draw_width, draw_height;
    LoadGraphDataFunc get_data;

    guint allocated;

    GdkRGBA *colors;
    gint **data;
    guint data_size;
    guint *pos;

    GtkWidget *main_widget;
    GtkWidget *frame, *box, *disp;
    cairo_surface_t *surface;
    int timer_index;

    gint show_frame;

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
    int last_clicked;

    float cpu_used_ratio;
    long  cpu_time [cpuload_n];
    long  cpu_last [cpuload_n];
    int   cpu_initialized;

    double loadavg1;

    guint64 memload_user;
    guint64 memload_cache;
    guint64 memload_total;

    float swapload_used_ratio;

    float diskload_used_ratio;
    gboolean nvme_diskstats;

    NetSpeed *netspeed_in;
    NetSpeed *netspeed_out;
    guint net_threshold1;
    guint net_threshold2;
    guint net_threshold3;
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
