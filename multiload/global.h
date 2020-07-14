#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <mate-panel-applet.h>

G_BEGIN_DECLS

#define NCPUSTATES 5
#define NGRAPHS 6
#define MIN_NET_THRESHOLD1 10
#define MIN_NET_THRESHOLD2 11
#define MIN_NET_THRESHOLD3 12
#define MAX_NET_THRESHOLD1  999999998
#define MAX_NET_THRESHOLD2  999999999
#define MAX_NET_THRESHOLD3 1000000000

typedef struct _MultiloadApplet MultiloadApplet;
typedef struct _LoadGraph LoadGraph;
typedef void (*LoadGraphDataFunc) (int, int [], LoadGraph *);

#include "netspeed.h"

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

    LoadGraph *graphs [NGRAPHS];

    GtkWidget *box;

    gboolean view_cpuload;
    gboolean view_memload;
    gboolean view_netload;
    gboolean view_swapload;
    gboolean view_loadavg;
    gboolean view_diskload;

    GtkWidget *about_dialog;
    GtkWidget *check_boxes [NGRAPHS];
    GtkWidget *prop_dialog;
    GtkWidget *notebook;
    int last_clicked;

    float cpu_used_ratio;
    long  cpu_time [NCPUSTATES];
    long  cpu_last [NCPUSTATES];
    int   cpu_initialized;

    double loadavg1;

    float memload_user_ratio;
    float memload_cache_ratio;

    float swapload_used_ratio;

    float diskload_used_ratio;

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
