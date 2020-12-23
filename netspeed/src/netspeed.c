 /*  netspeed.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Netspeed Applet was writen by Jörgen Scheibengruber <mfcn@gmx.de>
 *
 *  MATE Netspeed Applet migrated by Stefano Karapetsas <stefano@karapetsas.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "backend.h"
#include "netspeed.h"

#define GET_COLOR_CHOOSER(x) (GTK_COLOR_CHOOSER (gtk_builder_get_object (builder, (x))))
#define GET_DIALOG(x) (GTK_DIALOG (gtk_builder_get_object (builder, (x))))
#define GET_DRAWING_AREA(x) (GTK_DRAWING_AREA (gtk_builder_get_object (builder, (x))))
#define GET_WIDGET(x) (GTK_WIDGET (gtk_builder_get_object (builder, (x))))

 /* Icons for the interfaces */
static const char* const dev_type_icon [DEV_UNKNOWN + 1] = {
    /* FIXME: Need an actual loopback icon... */
    "reload",                    /* DEV_LO */
    "network-wired",             /* DEV_ETHERNET */
    "network-wireless",          /* DEV_WIRELESS */
    "modem",                     /* DEV_PPP */
    "mate-netspeed-plip",        /* DEV_PLIP */
    "mate-netspeed-plip",        /* DEV_SLIP */
    "network-workgroup",         /* DEV_UNKNOWN */
};

static const char* wireless_quality_icon [] = {
    "mate-netspeed-wireless-25",
    "mate-netspeed-wireless-50",
    "mate-netspeed-wireless-75",
    "mate-netspeed-wireless-100"
};

static const char IN_ICON[] = "go-down";
static const char OUT_ICON[] = "go-up";
static const char ERROR_ICON[] = "gtk-dialog-error";
static const char LOGO_ICON[] = "mate-netspeed-applet";

/* How many old in out values do we store?
 * The value actually shown in the applet is the average
 * of these values -> prevents the value from
 * "jumping around like crazy"
 */
#define OLD_VALUES          5
#define OLD_VALUES_DBL      5.0
#define GRAPH_VALUES      180
#define GRAPH_LINES         4
#define REFRESH_TIME     1000

/* A struct containing all the "global" data of the
 * applet
 */
struct _NetspeedApplet
{
    MatePanelApplet  parent;

    GtkWidget       *box;
    GtkWidget       *pix_box;
    GtkWidget       *speed_box;
    GtkWidget       *in_box;
    GtkWidget       *in_label;
    GtkWidget       *in_pix;
    GtkWidget       *out_box;
    GtkWidget       *out_label;
    GtkWidget       *out_pix;
    GtkWidget       *sum_box;
    GtkWidget       *sum_label;
    GtkWidget       *dev_pix;
    GtkWidget       *qual_pix;
    cairo_surface_t *qual_surfaces[4];
    gboolean         labels_dont_shrink;
    DevInfo         *devinfo;
    gboolean         device_has_changed;
    guint            timeout_id;
    char            *up_cmd;
    char            *down_cmd;
    gboolean         show_all_addresses;
    gboolean         show_sum;
    gboolean         show_bits;
    gboolean         change_icon;
    gboolean         auto_change_device;
    gboolean         show_icon;
    gboolean         show_quality_icon;
    GdkRGBA          in_color;
    GdkRGBA          out_color;
    int              width;
    /* details dialog */
    GtkDialog       *details;
    GtkDrawingArea  *drawingarea;
    GtkWidget       *ip_text;
    GtkWidget       *netmask_text;
    GtkWidget       *ptpip_text;
    GtkWidget       *ipv6_text;
    GtkWidget       *hwaddr_text;
    GtkWidget       *inbytes_text;
    GtkWidget       *outbytes_text;
    GtkWidget       *essid_text;
    GtkWidget       *signalbar;
#ifdef HAVE_NL
    GtkWidget       *station_text;
    GtkWidget       *channel_text;
    GtkWidget       *connected_time_text;
#endif
    GtkWidget       *ipv6_box;
    GtkWidget       *netlink_box;
    GtkWidget       *wireless_box;
    /* settings dialog */
    GtkDialog       *settings;
    GtkWidget       *network_device_combo;

    guint            index_old;
    guint64          in_old [OLD_VALUES];
    guint64          out_old [OLD_VALUES];
    double           max_graph;
    double           in_graph [GRAPH_VALUES];
    double           out_graph [GRAPH_VALUES];
    int              index_graph;
    GtkWidget       *connect_dialog;
    gboolean         show_tooltip;
    GtkIconTheme    *icon_theme;
    GSettings       *gsettings;
};

G_DEFINE_TYPE (NetspeedApplet, netspeed_applet, PANEL_TYPE_APPLET)

static void
update_tooltip (NetspeedApplet* applet);

static void
device_change_cb (GtkComboBox    *combo,
                  NetspeedApplet *applet);

/* Adds a Pango markup "foreground" to a bytestring
 */
static void
add_markup_fgcolor (char       **string,
                    const char  *color)
{
    char *tmp = *string;

    *string = g_strdup_printf ("<span foreground=\"%s\">%s</span>", color, tmp);
    g_free (tmp);
}

/* Change the icons according to the selected device
 */
static void
change_icons (NetspeedApplet *applet)
{
    cairo_surface_t *dev, *down;
    cairo_surface_t *in_arrow, *out_arrow;
    gint icon_scale;
    gint icon_size = mate_panel_applet_get_size (MATE_PANEL_APPLET (applet)) - 8;

    /* FIXME: Not all network icons include a high enough resolution, so to make them all
     * consistent, we cap them at 48px.*/
    icon_size = CLAMP (icon_size, 16, 48);

    icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (applet));

    /* If the user wants a different icon than current, we load it */
    if (applet->show_icon && applet->change_icon && applet->devinfo) {
        dev = gtk_icon_theme_load_surface (applet->icon_theme,
                                           dev_type_icon [applet->devinfo->type],
                                           icon_size, icon_scale, NULL, 0, NULL);
    } else {
        dev = gtk_icon_theme_load_surface (applet->icon_theme,
                                           dev_type_icon [DEV_UNKNOWN],
                                           icon_size, icon_scale, NULL, 0, NULL);
    }

    /* We need a fallback */
    if (dev == NULL)
        dev = gtk_icon_theme_load_surface (applet->icon_theme,
                                           dev_type_icon [DEV_UNKNOWN],
                                           icon_size, icon_scale, NULL, 0, NULL);

    in_arrow = gtk_icon_theme_load_surface (applet->icon_theme,
                                            IN_ICON,
                                            16, icon_scale, NULL, 0, NULL);

    out_arrow = gtk_icon_theme_load_surface (applet->icon_theme,
                                             OUT_ICON,
                                             16, icon_scale, NULL, 0, NULL);

    /* Set the windowmanager icon for the applet */
    gtk_window_set_default_icon_name (LOGO_ICON);

    gtk_image_set_from_surface (GTK_IMAGE (applet->out_pix), out_arrow);
    gtk_image_set_from_surface (GTK_IMAGE (applet->in_pix), in_arrow);
    cairo_surface_destroy (in_arrow);
    cairo_surface_destroy (out_arrow);

    if (applet->devinfo && applet->devinfo->running) {
        gtk_widget_show (applet->in_box);
        gtk_widget_show (applet->out_box);
    } else {
        cairo_t *cr;
        cairo_surface_t *copy;
        gint down_coords;

        gtk_widget_hide (applet->in_box);
        gtk_widget_hide (applet->out_box);

        /* We're not allowed to modify "dev" */
        copy = cairo_surface_create_similar (dev,
                                             cairo_surface_get_content (dev),
                                             cairo_image_surface_get_width (dev) / icon_scale,
                                             cairo_image_surface_get_height (dev) / icon_scale);
        cr = cairo_create (copy);
        cairo_set_source_surface (cr, dev, 0, 0);
        cairo_paint (cr);

        down = gtk_icon_theme_load_surface (applet->icon_theme,
                                            ERROR_ICON,
                                            icon_size, icon_scale, NULL, 0, NULL);

        down_coords = cairo_image_surface_get_width (copy) / icon_scale;
        cairo_scale (cr, 0.5, 0.5);
        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        cairo_set_source_surface (cr, down, down_coords, down_coords);
        cairo_paint (cr);

        cairo_surface_destroy (down);
        cairo_surface_destroy (dev);
        dev = copy;
    }

    if (applet->show_icon) {
        gtk_widget_show (applet->dev_pix);
        gtk_image_set_from_surface (GTK_IMAGE (applet->dev_pix), dev);
    } else {
        gtk_widget_hide (applet->dev_pix);
    }

    cairo_surface_destroy (dev);
}

/* Here some rearangement of the icons and the labels occurs
 * according to the panelsize and wether we show in and out
 * or just the sum
 */
static void
applet_change_size_or_orient (MatePanelApplet *applet_widget,
                              int              arg1,
                              NetspeedApplet  *applet)
{
    int size;
    MatePanelAppletOrient orient;

    g_assert (applet);

    size = mate_panel_applet_get_size (applet_widget);
    orient = mate_panel_applet_get_orient (applet_widget);

    g_object_ref (applet->pix_box);
    g_object_ref (applet->in_pix);
    g_object_ref (applet->in_label);
    g_object_ref (applet->out_pix);
    g_object_ref (applet->out_label);
    g_object_ref (applet->sum_label);

    if (applet->in_box) {
        gtk_container_remove (GTK_CONTAINER (applet->in_box), applet->in_label);
        gtk_container_remove (GTK_CONTAINER (applet->in_box), applet->in_pix);
        gtk_widget_destroy (applet->in_box);
    }
    if (applet->out_box) {
        gtk_container_remove (GTK_CONTAINER (applet->out_box), applet->out_label);
        gtk_container_remove (GTK_CONTAINER (applet->out_box), applet->out_pix);
        gtk_widget_destroy (applet->out_box);
    }
    if (applet->sum_box) {
        gtk_container_remove (GTK_CONTAINER (applet->sum_box), applet->sum_label);
        gtk_widget_destroy (applet->sum_box);
    }
    if (applet->box) {
        gtk_container_remove (GTK_CONTAINER (applet->box), applet->pix_box);
        gtk_widget_destroy (applet->box);
    }

    if (orient == MATE_PANEL_APPLET_ORIENT_LEFT || orient == MATE_PANEL_APPLET_ORIENT_RIGHT) {
        applet->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        applet->speed_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        if (size > 64) {
            applet->sum_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
            applet->in_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
            applet->out_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
        } else {
            applet->sum_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
            applet->in_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
            applet->out_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        }
        applet->labels_dont_shrink = FALSE;
    } else {
        applet->in_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
        applet->out_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
        applet->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
        applet->sum_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        if (size < 48) {
            applet->speed_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
            applet->labels_dont_shrink = TRUE;
        } else {
            applet->speed_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
            applet->labels_dont_shrink = !applet->show_sum;
        }
    }

    gtk_box_pack_start (GTK_BOX (applet->in_box), applet->in_pix, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (applet->in_box), applet->in_label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (applet->out_box), applet->out_pix, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (applet->out_box), applet->out_label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (applet->sum_box), applet->sum_label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (applet->box), applet->pix_box, FALSE, FALSE, 0);

    g_object_unref (applet->pix_box);
    g_object_unref (applet->in_pix);
    g_object_unref (applet->in_label);
    g_object_unref (applet->out_pix);
    g_object_unref (applet->out_label);
    g_object_unref (applet->sum_label);

    if (applet->show_sum) {
        gtk_box_pack_start (GTK_BOX (applet->speed_box), applet->sum_box, TRUE, TRUE, 0);
    } else {
        gtk_box_pack_start (GTK_BOX (applet->speed_box), applet->in_box, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (applet->speed_box), applet->out_box, TRUE, TRUE, 0);
    }
    gtk_box_pack_start (GTK_BOX (applet->box), applet->speed_box, TRUE, TRUE, 0);

    gtk_widget_show_all (applet->box);
    if (!applet->show_icon) {
        gtk_widget_hide (applet->dev_pix);
    }
    gtk_container_add (GTK_CONTAINER (applet), applet->box);

    change_icons (applet);
}

/* Change visibility of signal quality icon for wireless devices
 */
static void
change_quality_icon (NetspeedApplet *applet)
{
    if (applet->devinfo->type == DEV_WIRELESS &&
        applet->devinfo->up && applet->show_quality_icon) {
        gtk_widget_show (applet->qual_pix);
    } else {
        gtk_widget_hide (applet->qual_pix);
    }
}

static void
update_quality_icon (NetspeedApplet *applet)
{
    if (!applet->show_quality_icon) {
        return;
    }

    unsigned int q;

    q = (applet->devinfo->qual);
    q /= 25;
    q = MIN (q, 3); /* q out of range would crash when accessing qual_surfaces[q] */
    gtk_image_set_from_surface (GTK_IMAGE (applet->qual_pix), applet->qual_surfaces[q]);
}

static void
init_quality_surfaces (NetspeedApplet *applet)
{
    int i;
    cairo_surface_t *surface;
    gint icon_scale;

    /* FIXME: Add larger icon files. */
    gint icon_size = 24;

    icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (applet));

    for (i = 0; i < 4; i++) {
        if (applet->qual_surfaces[i])
            cairo_surface_destroy (applet->qual_surfaces[i]);

        surface = gtk_icon_theme_load_surface (applet->icon_theme,
                                               wireless_quality_icon[i],
                                               icon_size, icon_scale, NULL, 0, NULL);

        if (surface) {
            cairo_t *cr;

            applet->qual_surfaces[i] = cairo_surface_create_similar (surface,
                                                                     cairo_surface_get_content (surface),
                                                                     cairo_image_surface_get_width (surface) / icon_scale,
                                                                     cairo_image_surface_get_height (surface) / icon_scale);

            cr = cairo_create (applet->qual_surfaces[i]);
            cairo_set_source_surface (cr, surface, 0, 0);
            cairo_paint (cr);
            cairo_surface_destroy (surface);
        }
        else {
            applet->qual_surfaces[i] = NULL;
        }
    }
}

static void
icon_theme_changed_cb (GtkIconTheme *icon_theme,
                       gpointer      user_data)
{
    NetspeedApplet *applet = (NetspeedApplet*)user_data;

    init_quality_surfaces (user_data);
    if (applet->devinfo->type == DEV_WIRELESS && applet->devinfo->up)
        update_quality_icon (user_data);
    change_icons (user_data);
}

#define IEC_KIBI_DBL  1024.0
#define IEC_MEBI_DBL  1024.0*1024.0
#define IEC_GIBI_DBL  1024.0*1024.0*1024.0

static void
format_transfer_rate (gchar   *out,
                      double   bytes,
                      gboolean bits)
{
    const char *format;
    const char *unit;

    if (bits)
        bytes *= 8.0;

    if (bytes < IEC_KIBI_DBL) {
        format = "%.0f %s";
        unit = bits ? /* translators: bits (short) */ N_("bit/s"): /* translators: Bytes (short) */ N_("B/s");
    } else if (bytes < IEC_MEBI_DBL) {
        format = (bytes < (100.0 * IEC_KIBI_DBL)) ? "%.1f %s" : "%.0f %s";
        bytes /= IEC_KIBI_DBL;
        unit = bits ? /* translators: kibibits (short) */ N_("Kibit/s") : /* translators: Kibibytes (short) */ N_("KiB/s");
    } else if (bytes < IEC_GIBI_DBL) {
        format = "%.1f %s";
        bytes /= IEC_MEBI_DBL;
        unit = bits ? /* translators: Mebibit (short) */ N_("Mibit/s") : /* translators: Mebibyte (short) */ N_("MiB/s");
    } else {
        format = "%.1f %s";
        bytes /= IEC_GIBI_DBL;
        unit = bits ? /* translators: Gibibit (short) */ N_("Gibit/s") : /* translators: Gibibyte (short) */ N_("GiB/s");
    }

    g_snprintf (out, MAX_FORMAT_SIZE, format, bytes, gettext (unit));
}

/* Converts a number of bytes into a human
 * readable string - in [M/k]bytes[/s]
 * The string has to be freed
 */
static char*
bps_to_string (double   bytes,
               gboolean bits)
{
    char res[MAX_FORMAT_SIZE];
    format_transfer_rate (res, bytes, bits);
    return g_strdup (res);
}

/* Redraws the graph drawingarea
 * Some really black magic is going on in here ;-)
 */
static void
redraw_graph (NetspeedApplet *applet,
              cairo_t        *cr)
{
    GtkWidget *da = GTK_WIDGET (applet->drawingarea);
    GtkStyleContext *stylecontext = gtk_widget_get_style_context (da);
    GdkWindow *real_window = gtk_widget_get_window (da);
    GdkPoint in_points[GRAPH_VALUES], out_points[GRAPH_VALUES];
    PangoLayout *layout;
    PangoRectangle logical_rect;
    char *text;
    int i, offset, w, h;
    double max_val;
    double dash[2] = { 1.0, 2.0 };

    w = gdk_window_get_width (real_window);
    h = gdk_window_get_height (real_window);

    /* the graph hight should be: hight/2 <= applet->max_graph < hight */
    for (max_val = 1; max_val < applet->max_graph; max_val *= 2) ;

    /* calculate the polygons (GdkPoint[]) for the graphs */
    offset = 0;
    for (i = (applet->index_graph + 1) % GRAPH_VALUES; applet->in_graph[i] < 0; i = (i + 1) % GRAPH_VALUES)
        offset++;
    for (i = offset + 1; i < GRAPH_VALUES; i++)
    {
        int index = (applet->index_graph + i) % GRAPH_VALUES;
        out_points[i].x = in_points[i].x = ((w - 6) * i) / GRAPH_VALUES + 4;
        in_points[i].y = h - 6 - (int)((h - 8) * applet->in_graph[index] / max_val);
        out_points[i].y = h - 6 - (int)((h - 8) * applet->out_graph[index] / max_val);
    }
    in_points[offset].x = out_points[offset].x = ((w - 6) * offset) / GRAPH_VALUES + 4;
    in_points[offset].y = in_points[(offset + 1) % GRAPH_VALUES].y;
    out_points[offset].y = out_points[(offset + 1) % GRAPH_VALUES].y;

    /* draw the background */
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_rectangle (cr, 02, 2, w - 6, h - 6);
    cairo_fill (cr);

    cairo_set_line_width (cr, 1.0);
    cairo_set_dash (cr, dash, 2, 0);

    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_rectangle (cr, 2, 2, w - 6, h - 6);
    cairo_stroke (cr);

    for (i = 0; i < GRAPH_LINES; i++) {
        int y = 2 + ((h - 6) * i) / GRAPH_LINES;
        cairo_move_to (cr, 2, y);
        cairo_line_to (cr, w - 4, y);
    }
    cairo_stroke (cr);

    /* draw the polygons */
    cairo_set_dash (cr, dash, 0, 1);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

    gdk_cairo_set_source_rgba (cr, &applet->in_color);
    for (i = offset; i < GRAPH_VALUES; i++) {
        cairo_line_to (cr, in_points[i].x, in_points[i].y);
    }
    cairo_stroke (cr);

    gdk_cairo_set_source_rgba (cr, &applet->out_color);
    for (i = offset; i < GRAPH_VALUES; i++) {
        cairo_line_to (cr, out_points[i].x, out_points[i].y);
    }
    cairo_stroke (cr);

    text = bps_to_string (max_val, applet->show_bits);
    add_markup_fgcolor (&text, "black");
    layout = gtk_widget_create_pango_layout (da, NULL);
    pango_layout_set_markup (layout, text, -1);
    g_free (text);
    gtk_render_layout (stylecontext, cr, 3, 2, layout);
    g_object_unref (G_OBJECT (layout));

    text = bps_to_string (0.0, applet->show_bits);
    add_markup_fgcolor (&text, "black");
    layout = gtk_widget_create_pango_layout (da, NULL);
    pango_layout_set_markup (layout, text, -1);
    pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
    g_free (text);
    gtk_render_layout (stylecontext, cr, 3, h - 4 - logical_rect.height, layout);
    g_object_unref (G_OBJECT (layout));
}

static gboolean
set_applet_devinfo (NetspeedApplet *applet,
                    const char     *iface)
{
    DevInfo *info;

    get_device_info (iface, &info);

    if (info->running) {
        free_device_info (applet->devinfo);
        applet->devinfo = info;
        applet->device_has_changed = TRUE;
        return TRUE;
    }

    free_device_info (info);
    return FALSE;
}

/* Find the first available device, that is running and != lo */
static void
search_for_up_if (NetspeedApplet *applet)
{
    const gchar *default_route;
    GList *devices, *tmp;

    default_route = get_default_route ();

    if (default_route != NULL) {
        if (set_applet_devinfo (applet, default_route))
            return;
    }

    devices = get_available_devices ();
    for (tmp = devices; tmp; tmp = g_list_next (tmp)) {
        if (is_dummy_device (tmp->data))
            continue;
        if (set_applet_devinfo (applet, tmp->data))
            break;
    }
    free_devices_list (devices);
}

static char *
format_time (guint32 sec)
{
    char *res;
    char *m, *s;
    int seconds;
    int minutes;
    int hours;

    if (sec < 60)
        return g_strdup_printf (ngettext ("%'" G_GUINT32_FORMAT " second","%'" G_GUINT32_FORMAT " seconds", sec), sec);

    hours = (sec/3600);
    minutes = (sec -(3600*hours))/60;
    seconds = (sec -(3600*hours)-(minutes*60));

    m = g_strdup_printf (ngettext ("%'d minute", "%'d minutes", minutes), minutes);
    s = g_strdup_printf (ngettext ("%'d secon", "%'d seconds", seconds), seconds);
    if (sec < 60*60) {
        res = g_strconcat (m, ", ", s, NULL);
    } else {
        char *h;
        h = g_strdup_printf (ngettext ("%'d hour", "%'d hours", hours), hours);
        res = g_strconcat (h, ", ", m, ", ", s, NULL);
        g_free (h);
    }
    g_free (m);
    g_free (s);

    return res;
}

static char *
mac_addr_n2a (const unsigned char *hw)
{
    if (hw[6] || hw[7]) {
        return g_strdup_printf ("%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                                hw[0], hw[1], hw[2], hw[3],
                                hw[4], hw[5], hw[6], hw[7]);
    } else {
        return g_strdup_printf ("%02x:%02x:%02x:%02x:%02x:%02x",
                                hw[0], hw[1], hw[2],
                                hw[3], hw[4], hw[5]);
    }
}

static void
format_ipv4 (guint32  ip,
             char    *dest)
{
    inet_ntop (AF_INET, &ip, dest, INET_ADDRSTRLEN);
}

static void
format_ipv6 (const guint8  ip[16],
             char         *dest)
{
    inet_ntop (AF_INET6, ip, dest, INET6_ADDRSTRLEN);
}

static void
fill_details_dialog (NetspeedApplet *applet)
{
    char *text;
    char ipv4_text [INET_ADDRSTRLEN];

    if (applet->devinfo->ip) {
        format_ipv4 (applet->devinfo->ip, ipv4_text);
        text = ipv4_text;
    } else {
        text = _("none");
    }
    gtk_label_set_text (GTK_LABEL (applet->ip_text), text);

    if (applet->devinfo->netmask) {
        format_ipv4 (applet->devinfo->netmask, ipv4_text);
        text = ipv4_text;
    } else {
        text = _("none");
    }
    gtk_label_set_text (GTK_LABEL (applet->netmask_text), text);

    if (applet->devinfo->type != DEV_LO) {
        text = mac_addr_n2a (applet->devinfo->hwaddr);
        gtk_label_set_text (GTK_LABEL (applet->hwaddr_text), text);
        g_free (text);
    } else {
        gtk_label_set_text (GTK_LABEL (applet->hwaddr_text), _("none"));
    }

    if (applet->devinfo->ptpip) {
        format_ipv4 (applet->devinfo->ptpip, ipv4_text);
        text = ipv4_text;
    } else {
        text = _("none");
    }
    gtk_label_set_text (GTK_LABEL (applet->ptpip_text), text);

    /* check if we got an ipv6 address */
    GSList *ipv6_address_list = get_ip_address_list (applet->devinfo->name, FALSE);
    if (ipv6_address_list != NULL) {
        GSList *iterator;
        GString *string = NULL;

        for (iterator = ipv6_address_list; iterator; iterator = iterator->next) {
            if (string == NULL)
                string = g_string_new ((char*) iterator->data);
            else
                g_string_append_printf (string, "\n%s", (char*) iterator->data);
        }
        if (string != NULL) {
            gtk_label_set_text (GTK_LABEL (applet->ipv6_text), string->str);
            gtk_widget_show (applet->ipv6_box);
        }
        g_string_free (string, TRUE);
        g_slist_free_full (ipv6_address_list, g_free);
    } else {
        gtk_widget_hide (applet->ipv6_box);
    }

    if (applet->devinfo->type == DEV_WIRELESS) {
        float quality;

        /* _maybe_ we can add the encrypted icon between the essid and the signal bar. */

        quality = applet->devinfo->qual / 100.0f;
        if (quality > 1.0)
            quality = 1.0;

        text = g_strdup_printf ("%d %%", applet->devinfo->qual);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (applet->signalbar), quality);
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (applet->signalbar), text);
        g_free (text);

        gtk_label_set_text (GTK_LABEL (applet->essid_text),
                            applet->devinfo->essid);

#ifdef HAVE_NL
        if (applet->devinfo->running) {
            text = mac_addr_n2a (applet->devinfo->station_mac_addr);
            gtk_label_set_text (GTK_LABEL (applet->station_text), text);
            g_free (text);
        } else {
            gtk_label_set_text (GTK_LABEL (applet->station_text), _("unknown"));
        }

        gtk_label_set_text (GTK_LABEL (applet->channel_text),
                            applet->devinfo->channel ? applet->devinfo->channel : _("unknown"));

        text = format_time (applet->devinfo->connected_time);
        gtk_label_set_text (GTK_LABEL (applet->connected_time_text),
                            applet->devinfo->connected_time > 0 ? text : _("na"));
        g_free (text);

        gtk_widget_show (applet->netlink_box);
#else
        gtk_widget_hide (applet->netlink_box);
#endif /* HAVE_NL */
        gtk_widget_show (applet->wireless_box);
    } else {
        gtk_widget_hide (applet->wireless_box);
    }
}

/* Here happens the really interesting stuff */
static void
update_applet (NetspeedApplet *applet)
{
    guint64 indiff, outdiff;
    double inrate, outrate;
    char *inbytes, *outbytes;
    int i;
    DevInfo *oldinfo;

    if (!applet) return;

    /* First we try to figure out if the device has changed */
    oldinfo = applet->devinfo;
    get_device_info (oldinfo->name, &applet->devinfo);
    if (compare_device_info (applet->devinfo, oldinfo))
        applet->device_has_changed = TRUE;
    free_device_info (oldinfo);

    /* If the device has changed, reintialize stuff */
    if (applet->device_has_changed) {
        change_icons (applet);
        change_quality_icon (applet);

        for (i = 0; i < OLD_VALUES; i++) {
            applet->in_old[i] = applet->devinfo->rx;
            applet->out_old[i] = applet->devinfo->tx;
        }

        for (i = 0; i < GRAPH_VALUES; i++) {
            applet->in_graph[i] = -1;
            applet->out_graph[i] = -1;
        }

        applet->max_graph = 0;
        applet->index_graph = 0;

        if (applet->details) {
            fill_details_dialog (applet);
        }

        applet->device_has_changed = FALSE;
    }

    /* create the strings for the labels and tooltips */
    if (applet->devinfo->running) {
        if (applet->devinfo->rx < applet->in_old[applet->index_old])
            indiff = 0;
        else
            indiff = applet->devinfo->rx - applet->in_old[applet->index_old];

        if (applet->devinfo->tx < applet->out_old[applet->index_old])
            outdiff = 0;
        else
            outdiff = applet->devinfo->tx - applet->out_old[applet->index_old];

        inrate = (double)indiff / OLD_VALUES_DBL;
        outrate = (double)outdiff / OLD_VALUES_DBL;

        applet->in_graph[applet->index_graph] = inrate;
        applet->out_graph[applet->index_graph] = outrate;
        applet->max_graph = MAX (inrate, applet->max_graph);
        applet->max_graph = MAX (outrate, applet->max_graph);

        format_transfer_rate (applet->devinfo->rx_rate, inrate, applet->show_bits);
        format_transfer_rate (applet->devinfo->tx_rate, outrate, applet->show_bits);
        format_transfer_rate (applet->devinfo->sum_rate, inrate + outrate, applet->show_bits);
    } else {
        applet->devinfo->rx_rate[0] = '\0';
        applet->devinfo->tx_rate[0] = '\0';
        applet->devinfo->sum_rate[0] = '\0';
        applet->in_graph[applet->index_graph] = 0;
        applet->out_graph[applet->index_graph] = 0;
    }

    if (applet->devinfo->type == DEV_WIRELESS) {
        if (applet->devinfo->up)
            update_quality_icon (applet);

        if (applet->signalbar) {
            float quality;
            char *text;

            quality = applet->devinfo->qual / 100.0f;
            if (quality > 1.0)
                quality = 1.0;

            text = g_strdup_printf ("%d %%", applet->devinfo->qual);
            gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (applet->signalbar), quality);
            gtk_progress_bar_set_text (GTK_PROGRESS_BAR (applet->signalbar), text);
            g_free (text);
        }
#ifdef HAVE_NL
        /* Refresh the value of Connected Time */
        if (applet->connected_time_text) {
            char *text;

            text = format_time (applet->devinfo->connected_time);
            gtk_label_set_text (GTK_LABEL (applet->connected_time_text),
                                applet->devinfo->connected_time > 0 ? text : _("na"));
            g_free (text);
        }
#endif
    }

    update_tooltip (applet);

    /* Refresh the text of the labels */
    if (applet->show_sum) {
        gtk_label_set_text (GTK_LABEL (applet->sum_label),
                            applet->devinfo->sum_rate);
    } else {
        gtk_label_set_text (GTK_LABEL (applet->in_label),
                            applet->devinfo->rx_rate);
        gtk_label_set_text (GTK_LABEL (applet->out_label),
                            applet->devinfo->tx_rate);
    }

    /* Refresh the values of the Infodialog */
    if (applet->inbytes_text) {
        if (applet->show_bits)
            inbytes = g_format_size_full (applet->devinfo->rx*8,
                                          G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_BITS);
        else
            inbytes = g_format_size_full (applet->devinfo->rx,
                                          G_FORMAT_SIZE_IEC_UNITS);

        gtk_label_set_text (GTK_LABEL (applet->inbytes_text), inbytes);
        g_free (inbytes);
    }
    if (applet->outbytes_text) {
        if (applet->show_bits)
            outbytes = g_format_size_full (applet->devinfo->tx*8,
                                           G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_BITS);
        else
            outbytes = g_format_size_full (applet->devinfo->tx,
                                           G_FORMAT_SIZE_IEC_UNITS);

        gtk_label_set_text (GTK_LABEL (applet->outbytes_text), outbytes);
        g_free (outbytes);
    }

    /* Redraw the graph of the Infodialog */
    if (applet->drawingarea)
        gtk_widget_queue_draw (GTK_WIDGET (applet->drawingarea));

    /* Save old values... */
    applet->in_old  [applet->index_old] = applet->devinfo->rx;
    applet->out_old [applet->index_old] = applet->devinfo->tx;
    applet->index_old = (applet->index_old + 1) % OLD_VALUES;

    /* Move the graphindex. Check if we can scale down again */
    applet->index_graph = (applet->index_graph + 1) % GRAPH_VALUES;
    if (applet->index_graph % 20 == 0) {
        double max = 0;

        for (i = 0; i < GRAPH_VALUES; i++) {
            max = MAX (max, applet->in_graph[i]);
            max = MAX (max, applet->out_graph[i]);
        }
        applet->max_graph = max;
    }

    /* Always follow the default route */
    if (applet->auto_change_device) {
        gboolean change_device_now = !applet->devinfo->running;

        if (!change_device_now) {
            const gchar *default_route;

            default_route = get_default_route ();
            change_device_now = (default_route != NULL &&
                                 strcmp (default_route, applet->devinfo->name));
        }
        if (change_device_now) {
            search_for_up_if (applet);
        }
    }
}

static gboolean
timeout_function (NetspeedApplet *applet)
{
    if (!applet)
        return FALSE;
    if (!applet->timeout_id)
        return FALSE;

    update_applet (applet);
    return TRUE;
}

/* Display a section of netspeed help
 */
static void
display_help (GtkWidget   *dialog,
              const gchar *section)
{
    GError *error = NULL;
    gboolean ret;
    char *uri;

    if (section)
        uri = g_strdup_printf ("help:mate-netspeed-applet/%s", section);
    else
        uri = g_strdup ("help:mate-netspeed-applet");

    ret = gtk_show_uri_on_window (NULL,
                                  uri,
                                  gtk_get_current_event_time (),
                                  &error);
    g_free (uri);

    if (ret == FALSE) {
        GtkWidget *error_dialog = gtk_message_dialog_new (NULL,
                                                          GTK_DIALOG_MODAL,
                                                          GTK_MESSAGE_ERROR,
                                                          GTK_BUTTONS_OK,
                                                          _("There was an error displaying help:\n%s"),
                                                          error->message);
        g_signal_connect (error_dialog, "response",
                          G_CALLBACK (gtk_widget_destroy), NULL);

        gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
        gtk_window_set_screen (GTK_WINDOW (error_dialog), gtk_widget_get_screen (dialog));
        gtk_widget_show (error_dialog);
        g_error_free (error);
    }
}

/* Opens gnome help application
 */
static void
help_cb (GtkAction      *action,
         NetspeedApplet *applet)
{
    display_help (GTK_WIDGET (applet), NULL);
}

/* Just the about window... If it's already open, just fokus it
 */
static void
about_cb (GtkAction *action,
          gpointer   data)
{
    const char *authors[] =
    {
        "Jörgen Scheibengruber <mfcn@gmx.de>",
        "Dennis Cranston <dennis_cranston@yahoo.com>",
        "Pedro Villavicencio Garrido <pvillavi@gnome.org>",
        "Benoît Dejean <benoit@placenet.org>",
        "Stefano Karapetsas <stefano@karapetsas.com>",
        "Perberos <perberos@gmail.com>",
        NULL
    };

    gtk_show_about_dialog (NULL,
                           "title", _("About MATE Netspeed"),
                           "version", VERSION,
                           "copyright", _("Copyright \xc2\xa9 2002-2003 Jörgen Scheibengruber\n"
                                          "Copyright \xc2\xa9 2011-2014 Stefano Karapetsas\n"
                                          "Copyright \xc2\xa9 2015-2020 MATE developers"),
                           "comments", _("A little applet that displays some information on the traffic on the specified network device"),
                           "authors", authors,
                           "documenters", NULL,
                           "translator-credits", _("translator-credits"),
                           "website", "http://www.mate-desktop.org/",
                           "logo-icon-name", LOGO_ICON,
                            NULL);
}

/* this basically just retrieves the new devicestring
 * and then calls applet_device_change () and change_icons ()
 */
static void
device_change_cb (GtkComboBox    *combo,
                  NetspeedApplet *applet)
{
    GList *devices;
    int i, active;

    g_assert (combo);
    devices = g_object_get_data (G_OBJECT (combo), "devices");
    active = gtk_combo_box_get_active (combo);
    g_assert (active > -1);

    if (0 == active) {
        if (applet->auto_change_device)
            return;
        applet->auto_change_device = TRUE;
    } else {
        applet->auto_change_device = FALSE;
        for (i = 1; i < active; i++) {
            devices = g_list_next (devices);
        }
        if (g_str_equal (devices->data, applet->devinfo->name))
            return;
        free_device_info (applet->devinfo);
        get_device_info (devices->data, &applet->devinfo);
    }

    applet->device_has_changed = TRUE;
    update_applet (applet);
}

/* Handle preference dialog response event
 */
static void
pref_response_cb (GtkDialog *dialog,
                  gint       id,
                  gpointer   data)
{
    NetspeedApplet *applet = data;

    if (id == GTK_RESPONSE_HELP) {
        display_help (GTK_WIDGET (dialog), "netspeed_applet-settings");
        return;
    }
    g_settings_delay (applet->gsettings);
    g_settings_set_string (applet->gsettings, "device", applet->devinfo->name);
    g_settings_set_boolean (applet->gsettings, "auto-change-device", applet->auto_change_device);
    g_settings_apply (applet->gsettings);

    gtk_widget_destroy (GTK_WIDGET (applet->settings));
    applet->settings = NULL;
}

/* Called when the showalladdresses checkbutton is toggled...
 */
static void
showalladdresses_change_cb (GtkToggleButton *togglebutton,
                            NetspeedApplet  *applet)
{
    applet->show_all_addresses = gtk_toggle_button_get_active (togglebutton);
}

/* Called when the showsum checkbutton is toggled...
 */
static void
showsum_change_cb (GtkToggleButton *togglebutton,
                   NetspeedApplet  *applet)
{
    applet->show_sum = gtk_toggle_button_get_active (togglebutton);
    applet_change_size_or_orient (MATE_PANEL_APPLET (applet), -1, (gpointer) applet);
    change_icons (applet);
}

/* Called when the showbits checkbutton is toggled...
 */
static void
showbits_change_cb (GtkToggleButton  *togglebutton,
                    NetspeedApplet   *applet)
{
    applet->show_bits = gtk_toggle_button_get_active (togglebutton);
}

/* Called when the showicon checkbutton is toggled...
 */
static void
showicon_change_cb (GtkToggleButton *togglebutton,
                    NetspeedApplet  *applet)
{
    applet->show_icon = gtk_toggle_button_get_active (togglebutton);
    change_icons (applet);
}

/* Called when the showqualityicon checkbutton is toggled...
 */
static void
showqualityicon_change_cb (GtkToggleButton *togglebutton,
                           NetspeedApplet  *applet)
{
    applet->show_quality_icon = gtk_toggle_button_get_active (togglebutton);
    change_quality_icon (applet);
}

/* Called when the changeicon checkbutton is toggled...
 */
static void
changeicon_change_cb (GtkToggleButton *togglebutton,
                      NetspeedApplet  *applet)
{
    applet->change_icon = gtk_toggle_button_get_active (togglebutton);
    change_icons (applet);
}

/* Creates the settings dialog
 * After its been closed, take the new values and store
 * them in the gsettings database
 */
static void
settings_cb (GtkAction *action,
             gpointer   data)
{
    GtkBuilder *builder;
    NetspeedApplet *applet = (NetspeedApplet*)data;
    GList *ptr, *devices;
    int i, active = -1;

    g_assert (applet);

    if (applet->settings)
    {
        gtk_window_present (GTK_WINDOW (applet->settings));
        return;
    }

    builder = gtk_builder_new_from_resource (NETSPEED_RESOURCE_PATH "netspeed-preferences.ui");

    applet->settings = GET_DIALOG ("preferences_dialog");
    applet->network_device_combo = GET_WIDGET ("network_device_combo");

    gtk_window_set_screen (GTK_WINDOW (applet->settings),
                           gtk_widget_get_screen (GTK_WIDGET (applet->settings)));

    gtk_dialog_set_default_response (GTK_DIALOG (applet->settings), GTK_RESPONSE_CLOSE);

    g_settings_bind (applet->gsettings, "show-all-addresses",
                     gtk_builder_get_object (builder, "show_all_addresses_checkbutton"),
                     "active", G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (applet->gsettings, "show-sum",
                     gtk_builder_get_object (builder, "show_sum_checkbutton"),
                     "active", G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (applet->gsettings, "show-bits",
                     gtk_builder_get_object (builder, "show_bits_checkbutton"),
                     "active", G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (applet->gsettings, "show-icon",
                     gtk_builder_get_object (builder, "show_icon_checkbutton"),
                     "active", G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (applet->gsettings, "show-quality-icon",
                     gtk_builder_get_object (builder, "show_quality_icon_checkbutton"),
                     "active", G_SETTINGS_BIND_DEFAULT);

    g_settings_bind (applet->gsettings, "change-icon",
                     gtk_builder_get_object (builder, "change_icon_checkbutton"),
                     "active", G_SETTINGS_BIND_DEFAULT);

    /* Default means device with default route set */
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (applet->network_device_combo),
                                    _("Default"));
    ptr = devices = get_available_devices ();
    for (i = 0; ptr; ptr = g_list_next (ptr)) {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (applet->network_device_combo),
                                        ptr->data);
        if (g_str_equal (ptr->data, applet->devinfo->name))
            active = (i + 1);
        ++i;
    }
    if (active < 0 || applet->auto_change_device) {
        active = 0;
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (applet->network_device_combo), active);
    g_object_set_data_full (G_OBJECT (applet->network_device_combo), "devices",
                            devices, (GDestroyNotify)free_devices_list);

    /* signals */
    gtk_builder_add_callback_symbols (builder,
                                      "on_network_device_combo_changed", G_CALLBACK (device_change_cb),
                                      "on_show_all_addresses_checkbutton_toggled", G_CALLBACK (showalladdresses_change_cb),
                                      "on_show_sum_checkbutton_toggled", G_CALLBACK (showsum_change_cb),
                                      "on_show_bits_checkbutton_toggled", G_CALLBACK (showbits_change_cb),
                                      "on_change_icon_checkbutton_toggled", G_CALLBACK (changeicon_change_cb),
                                      "on_show_icon_checkbutton_toggled", G_CALLBACK (showicon_change_cb),
                                      "on_show_quality_icon_checkbutton_toggled", G_CALLBACK (showqualityicon_change_cb),
                                      "on_preferences_dialog_response", G_CALLBACK (pref_response_cb),
                                      NULL);
    gtk_builder_connect_signals (builder, applet);

    g_object_unref (builder);

    gtk_widget_show_all (GTK_WIDGET (applet->settings));
}

static gboolean
da_draw (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
    NetspeedApplet *applet = (NetspeedApplet*) data;

    redraw_graph (applet, cr);

    return FALSE;
}

static void
incolor_changed_cb (GtkColorChooser *button,
                    gpointer         data)
{
    NetspeedApplet *applet = (NetspeedApplet*) data;
    GdkRGBA color;
    gchar *string;

    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
    applet->in_color = color;

    string = gdk_rgba_to_string (&color);
    g_settings_set_string (applet->gsettings, "in-color", string);
    g_free (string);
}

static void
outcolor_changed_cb (GtkColorChooser *button,
                     gpointer data)
{
    NetspeedApplet *applet = (NetspeedApplet*)data;
    GdkRGBA color;
    gchar *string;

    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
    applet->out_color = color;

    string = gdk_rgba_to_string (&color);
    g_settings_set_string (applet->gsettings, "out-color", string);
    g_free (string);
}

/* Handle info dialog response event
 */
static void
info_response_cb (GtkDialog      *dialog,
                  gint            id,
                  NetspeedApplet *applet)
{

    if (id == GTK_RESPONSE_HELP) {
        display_help (GTK_WIDGET (dialog), "netspeed_applet-details");
        return;
    }

    gtk_widget_destroy (GTK_WIDGET (applet->details));

    applet->details       = NULL;
    applet->drawingarea   = NULL;
    applet->ip_text       = NULL;
    applet->netmask_text  = NULL;
    applet->ptpip_text    = NULL;
    applet->ipv6_text     = NULL;
    applet->hwaddr_text   = NULL;
    applet->inbytes_text  = NULL;
    applet->outbytes_text = NULL;
    applet->essid_text    = NULL;
    applet->signalbar     = NULL;
#ifdef HAVE_NL
    applet->station_text        = NULL;
    applet->channel_text        = NULL;
    applet->connected_time_text = NULL;
#endif /* HAVE_NL */
    applet->ipv6_box      = NULL;
    applet->netlink_box   = NULL;
    applet->wireless_box  = NULL;
}

/* Creates the details dialog
 */
static void
showinfo_cb (GtkAction *action,
             gpointer   data)
{
    GtkBuilder *builder;
    NetspeedApplet *applet = (NetspeedApplet*)data;

    g_assert (applet);

    if (applet->details) {
        gtk_window_present (GTK_WINDOW (applet->details));
        return;
    }

    builder = gtk_builder_new_from_resource (NETSPEED_RESOURCE_PATH "netspeed-details.ui");

    applet->details       = GET_DIALOG ("dialog");
    applet->drawingarea   = GET_DRAWING_AREA ("drawingarea");

    applet->ip_text       = GET_WIDGET ("ip_text");
    applet->netmask_text  = GET_WIDGET ("netmask_text");
    applet->ptpip_text    = GET_WIDGET ("ptpip_text");
    applet->ipv6_text     = GET_WIDGET ("ipv6_text");
    applet->hwaddr_text   = GET_WIDGET ("hwaddr_text");
    applet->inbytes_text  = GET_WIDGET ("inbytes_text");
    applet->outbytes_text = GET_WIDGET ("outbytes_text");
    applet->essid_text    = GET_WIDGET ("essid_text");
    applet->signalbar     = GET_WIDGET ("signalbar");
#ifdef HAVE_NL
    applet->station_text  = GET_WIDGET ("station_text");
    applet->channel_text  = GET_WIDGET ("channel_text");

    applet->connected_time_text = GET_WIDGET ("connected_time_text");
#endif /* HAVE_NL */
    applet->ipv6_box      = GET_WIDGET ("ipv6_box");
    applet->netlink_box   = GET_WIDGET ("netlink_box");
    applet->wireless_box  = GET_WIDGET ("wireless_box");

    gtk_color_chooser_set_rgba (GET_COLOR_CHOOSER ("incolor_sel"),  &applet->in_color);
    gtk_color_chooser_set_rgba (GET_COLOR_CHOOSER ("outcolor_sel"),  &applet->out_color);

    fill_details_dialog (applet);

    gtk_builder_add_callback_symbols (builder,
                                      "on_drawingarea_draw", G_CALLBACK (da_draw),
                                      "on_incolor_sel_color_set", G_CALLBACK (incolor_changed_cb),
                                      "on_outcolor_sel_color_set", G_CALLBACK (outcolor_changed_cb),
                                      "on_dialog_response", G_CALLBACK (info_response_cb),
                                      NULL);
    gtk_builder_connect_signals (builder, applet);

    g_object_unref (builder);

    gtk_window_present (GTK_WINDOW (applet->details));
}

static const GtkActionEntry netspeed_applet_menu_actions [] = {
        { "NetspeedAppletDetails", "dialog-information", N_("Device _Details"),
          NULL, NULL, G_CALLBACK (showinfo_cb) },
        { "NetspeedAppletProperties", "document-properties", N_("Preferences..."),
          NULL, NULL, G_CALLBACK (settings_cb) },
        { "NetspeedAppletHelp", "help-browser", N_("Help"),
          NULL, NULL, G_CALLBACK (help_cb) },
        { "NetspeedAppletAbout", "help-about", N_("About..."),
          NULL, NULL, G_CALLBACK (about_cb) }
};

/* Block the size_request signal emit by the label if the
 * text changes. Only if the label wants to grow, we give
 * permission. This will eventually result in the maximal
 * size of the applet and prevents the icons and labels from
 * "jumping around" in the mate_panel which looks uggly
 */
static void
label_size_allocate_cb (GtkWidget      *widget,
                        GtkAllocation  *allocation,
                        NetspeedApplet *applet)
{
    if (applet->labels_dont_shrink) {
        if (allocation->width <= applet->width)
            allocation->width = applet->width;
        else
            applet->width = allocation->width;
    }
}

static gboolean
applet_button_press (GtkWidget      *widget,
                     GdkEventButton *event,
                     NetspeedApplet *applet)
{
    if (event->button == 1) {
        GError *error = NULL;

        if (applet->connect_dialog) {
            gtk_window_present (GTK_WINDOW (applet->connect_dialog));
            return FALSE;
        }

        if (applet->up_cmd && applet->down_cmd) {
            char *question;
            int   response;

            if (applet->devinfo->up)
                question = g_strdup_printf (_("Do you want to disconnect %s now?"),
                                            applet->devinfo->name);
            else
                question = g_strdup_printf (_("Do you want to connect %s now?"),
                                            applet->devinfo->name);

            applet->connect_dialog =
                gtk_message_dialog_new (NULL,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                        "%s", question);

            response = gtk_dialog_run (GTK_DIALOG (applet->connect_dialog));
            gtk_widget_destroy (applet->connect_dialog);
            applet->connect_dialog = NULL;
            g_free (question);

            if (response == GTK_RESPONSE_YES) {
                GtkWidget *dialog;
                char      *command;

                command = g_strdup_printf ("%s %s",
                                           applet->devinfo->up ? applet->down_cmd : applet->up_cmd,
                                           applet->devinfo->name);

                if (!g_spawn_command_line_async (command, &error)) {
                    dialog = gtk_message_dialog_new_with_markup (NULL,
                                                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                 GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                                 _("<b>Running command %s failed</b>\n%s"), command, error->message);
                    gtk_dialog_run (GTK_DIALOG (dialog));
                    gtk_widget_destroy (dialog);

                    g_error_free (error);
                }

                g_free (command);
            }
        }
    }

    return FALSE;
}

/* Frees the applet and all the data it contains
 * Removes the timeout_cb
 */
static void
applet_destroy (MatePanelApplet *applet_widget,
                NetspeedApplet  *applet)
{
    g_assert (applet);

    if (applet->icon_theme != NULL) {
        g_signal_handlers_disconnect_by_func (applet->icon_theme,
                                              icon_theme_changed_cb,
                                              applet);
        applet->icon_theme = NULL;
    }

    g_source_remove (applet->timeout_id);
    applet->timeout_id = 0;

    if (applet->up_cmd)
        g_free (applet->up_cmd);
    if (applet->down_cmd)
        g_free (applet->down_cmd);
    if (applet->gsettings)
        g_object_unref (applet->gsettings);

    /* Should never be NULL */
    free_device_info (applet->devinfo);
    g_free (applet);
    return;
}

static void
update_tooltip (NetspeedApplet* applet)
{
    GString* tooltip;

    if (!applet->show_tooltip)
        return;

    tooltip = g_string_new ("");

    if (!applet->devinfo->running)
        g_string_printf (tooltip, _("%s is down"), applet->devinfo->name);
    else {
        char ipv4_text [INET_ADDRSTRLEN];
        char ipv6_text [INET6_ADDRSTRLEN];
        char *ip;

        if (applet->show_all_addresses) {
            format_ipv6 (applet->devinfo->ipv6, ipv6_text);
            if (applet->devinfo->ip) {
                format_ipv4 (applet->devinfo->ip, ipv4_text);
                if (strlen (ipv6_text) > 2) {
                    g_string_printf (tooltip,
                                     _("%s: %s and %s"),
                                     applet->devinfo->name,
                                     ipv4_text,
                                     ipv6_text);
                } else {
                    g_string_printf (tooltip,
                                     _("%s: %s"),
                                     applet->devinfo->name,
                                     ipv4_text);
                }
            } else {
                if (strlen (ipv6_text) > 2) {
                    g_string_printf (tooltip,
                                     _("%s: %s"),
                                     applet->devinfo->name,
                                     ipv6_text);
                } else {
                    g_string_printf (tooltip,
                                     _("%s: has no ip"),
                                     applet->devinfo->name);
                }
            }
        } else {
            if (applet->devinfo->ip) {
                format_ipv4 (applet->devinfo->ip, ipv4_text);
                ip = ipv4_text;
            } else {
                format_ipv6 (applet->devinfo->ipv6, ipv6_text);
                if (strlen (ipv6_text) > 2) {
                    ip = ipv6_text;
                } else {
                    ip = _("has no ip");
                }
            }
            g_string_printf (tooltip,
                             _("%s: %s"),
                             applet->devinfo->name,
                             ip);
        }

        if (applet->show_sum) {
            g_string_append_printf (tooltip,
                                    _("\nin: %s out: %s"),
                                    applet->devinfo->rx_rate,
                                    applet->devinfo->tx_rate);
        } else {
            g_string_append_printf (tooltip,
                                    _("\nsum: %s"),
                                    applet->devinfo->sum_rate);
        }

#ifdef HAVE_NL
        if (applet->devinfo->type == DEV_WIRELESS)
            g_string_append_printf (tooltip,
                                    _("\nESSID: %s\nRSSI: %d dBm\nRX Bitrate: %s\nTX Bitrate: %s"),
                                    applet->devinfo->essid ? applet->devinfo->essid : _("unknown"),
                                    applet->devinfo->rssi,
                                    applet->devinfo->rx_bitrate,
                                    applet->devinfo->tx_bitrate);
#endif /* HAVE_NL */
#ifdef HAVE_IW
        if (applet->devinfo->type == DEV_WIRELESS)
            g_string_append_printf (tooltip,
                                    _("\nESSID: %s\nStrength: %d %%"),
                                    applet->devinfo->essid ? applet->devinfo->essid : _("unknown"),
                                    applet->devinfo->qual);
#endif /* HAVE_IW */
    }

    gtk_widget_set_tooltip_text (GTK_WIDGET (applet), tooltip->str);
    gtk_widget_trigger_tooltip_query (GTK_WIDGET (applet));
    g_string_free (tooltip, TRUE);
}


static gboolean
netspeed_applet_enter_cb (GtkWidget        *widget,
                          GdkEventCrossing *event,
                          gpointer          data)
{
    NetspeedApplet *applet = data;

    applet->show_tooltip = TRUE;
    update_tooltip (applet);

    return TRUE;
}

static gboolean
netspeed_applet_leave_cb (GtkWidget       *widget,
                        GdkEventCrossing *event,
                        gpointer          data)
{
    NetspeedApplet *applet = data;

    applet->show_tooltip = FALSE;
    return TRUE;
}

static void
netspeed_applet_class_init (NetspeedAppletClass *netspeed_class)
{
}

static void
netspeed_applet_init (NetspeedApplet *netspeed)
{
}

/* The "main" function of the applet
 */
static gboolean
netspeed_applet_factory (MatePanelApplet *applet_widget,
                         const gchar     *iid,
                         gpointer         data)
{
    NetspeedApplet *applet;
    int i;
    GtkWidget *spacer, *spacer_box;
    GtkActionGroup *action_group;

    if (strcmp (iid, "NetspeedApplet"))
        return FALSE;

    glibtop_init ();
    g_set_application_name (_("MATE Netspeed"));

    applet = NETSPEED_APPLET (applet_widget);
    applet->icon_theme = gtk_icon_theme_get_default ();

    /* Set the default colors of the graph
    */
    applet->in_color.red = 0xdf00;
    applet->in_color.green = 0x2800;
    applet->in_color.blue = 0x4700;
    applet->out_color.red = 0x3700;
    applet->out_color.green = 0x2800;
    applet->out_color.blue = 0xdf00;

    for (i = 0; i < GRAPH_VALUES; i++)
    {
        applet->in_graph[i] = -1;
        applet->out_graph[i] = -1;
    }

    applet->gsettings = mate_panel_applet_settings_new (applet_widget, "org.mate.panel.applet.netspeed");

    /* Get stored settings from gsettings
     */
    applet->show_all_addresses = g_settings_get_boolean (applet->gsettings, "show-all-addresses");
    applet->show_sum = g_settings_get_boolean (applet->gsettings, "show-sum");
    applet->show_bits = g_settings_get_boolean (applet->gsettings, "show-bits");
    applet->show_icon = g_settings_get_boolean (applet->gsettings, "show-icon");
    applet->show_quality_icon = g_settings_get_boolean (applet->gsettings, "show-quality-icon");
    applet->change_icon = g_settings_get_boolean (applet->gsettings, "change-icon");
    applet->auto_change_device = g_settings_get_boolean (applet->gsettings, "auto-change-device");

    char *tmp = NULL;
    tmp = g_settings_get_string (applet->gsettings, "device");
    if (tmp && strcmp (tmp, "")) {
        get_device_info (tmp, &applet->devinfo);
        g_free (tmp);
    }

    tmp = g_settings_get_string (applet->gsettings, "up-command");
    if (tmp && strcmp (tmp, "")) {
        applet->up_cmd = g_strdup (tmp);
        g_free (tmp);
    }

    tmp = g_settings_get_string (applet->gsettings, "down-command");
    if (tmp && strcmp (tmp, "")) {
        applet->down_cmd = g_strdup (tmp);
        g_free (tmp);
    }

    tmp = g_settings_get_string (applet->gsettings, "in-color");
    if (tmp) {
        gdk_rgba_parse (&applet->in_color, tmp);
        g_free (tmp);
    }

    tmp = g_settings_get_string (applet->gsettings, "out-color");
    if (tmp) {
        gdk_rgba_parse (&applet->out_color, tmp);
        g_free (tmp);
    }

    if (!applet->devinfo) {
        GList *ptr, *devices = get_available_devices ();
        ptr = devices;
        while (ptr) {
            if (!g_str_equal (ptr->data, "lo")) {
                get_device_info (ptr->data, &applet->devinfo);
                break;
            }
            ptr = g_list_next (ptr);
        }
        free_devices_list (devices);
    }
    if (!applet->devinfo)
        get_device_info ("lo", &applet->devinfo);

    applet->device_has_changed = TRUE;

    applet->in_label = gtk_label_new ("");
    applet->out_label = gtk_label_new ("");
    applet->sum_label = gtk_label_new ("");

    applet->in_pix = gtk_image_new ();
    applet->out_pix = gtk_image_new ();
    applet->dev_pix = gtk_image_new ();
    applet->qual_pix = gtk_image_new ();

    applet->pix_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    spacer = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (applet->pix_box), spacer, TRUE, TRUE, 0);
    spacer = gtk_label_new ("");
    gtk_box_pack_end (GTK_BOX (applet->pix_box), spacer, TRUE, TRUE, 0);

    spacer_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start (GTK_BOX (applet->pix_box), spacer_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (spacer_box), applet->qual_pix, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (spacer_box), applet->dev_pix, FALSE, FALSE, 0);

    init_quality_surfaces (applet);

    applet_change_size_or_orient (applet_widget, -1, (gpointer)applet);
    gtk_widget_show_all (GTK_WIDGET (applet_widget));
    update_applet (applet);

    mate_panel_applet_set_flags (applet_widget, MATE_PANEL_APPLET_EXPAND_MINOR);

    applet->timeout_id = g_timeout_add (REFRESH_TIME,
                                        (GSourceFunc)timeout_function,
                                        (gpointer)applet);

    g_signal_connect (G_OBJECT (applet_widget), "change_size",
                      G_CALLBACK (applet_change_size_or_orient),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet->icon_theme), "changed",
                      G_CALLBACK (icon_theme_changed_cb),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet_widget), "change_orient",
                      G_CALLBACK (applet_change_size_or_orient),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet->in_label), "size_allocate",
                      G_CALLBACK (label_size_allocate_cb),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet->out_label), "size_allocate",
                      G_CALLBACK (label_size_allocate_cb),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet->sum_label), "size_allocate",
                      G_CALLBACK (label_size_allocate_cb),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet_widget), "destroy",
                      G_CALLBACK (applet_destroy),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet_widget), "button-press-event",
                      G_CALLBACK (applet_button_press),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet_widget), "leave_notify_event",
                      G_CALLBACK (netspeed_applet_leave_cb),
                      (gpointer)applet);

    g_signal_connect (G_OBJECT (applet_widget), "enter_notify_event",
                      G_CALLBACK (netspeed_applet_enter_cb),
                      (gpointer)applet);

    action_group = gtk_action_group_new ("Netspeed Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group,
                                  netspeed_applet_menu_actions,
                                  G_N_ELEMENTS (netspeed_applet_menu_actions),
                                  applet);

    mate_panel_applet_setup_menu_from_resource (applet_widget,
                                                NETSPEED_RESOURCE_PATH "netspeed-menu.xml",
                                                action_group);

    g_object_unref (action_group);

    return TRUE;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("NetspeedAppletFactory",
                                       NETSPEED_TYPE_APPLET,
                                       "NetspeedApplet",
                                       netspeed_applet_factory,
                                       NULL)
