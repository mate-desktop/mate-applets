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
#include "netspeed-preferences.h"

#include "netspeed.h"

#define GET_COLOR_CHOOSER(x) (GTK_COLOR_CHOOSER (gtk_builder_get_object (builder, (x))))
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
    GtkWidget       *details;
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
    GtkWidget       *preferences;

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
    GSettings       *settings;
};

G_DEFINE_TYPE (NetspeedApplet, netspeed_applet, PANEL_TYPE_APPLET)

static void
update_tooltip (NetspeedApplet *netspeed);

GSettings *
netspeed_applet_get_settings (NetspeedApplet *netspeed)
{
    return netspeed->settings;
}

const gchar *
netspeed_applet_get_current_device_name (NetspeedApplet *netspeed)
{
    return netspeed->devinfo->name;
}

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
change_icons (NetspeedApplet *netspeed)
{
    cairo_surface_t *dev, *down;
    cairo_surface_t *in_arrow, *out_arrow;
    gint icon_scale;
    gint icon_size = mate_panel_applet_get_size (MATE_PANEL_APPLET (netspeed)) - 8;

    /* FIXME: Not all network icons include a high enough resolution, so to make them all
     * consistent, we cap them at 48px.*/
    icon_size = CLAMP (icon_size, 16, 48);

    icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (netspeed));

    /* If the user wants a different icon than current, we load it */
    if (netspeed->show_icon && netspeed->change_icon && netspeed->devinfo) {
        dev = gtk_icon_theme_load_surface (netspeed->icon_theme,
                                           dev_type_icon [netspeed->devinfo->type],
                                           icon_size, icon_scale, NULL, 0, NULL);
    } else {
        dev = gtk_icon_theme_load_surface (netspeed->icon_theme,
                                           dev_type_icon [DEV_UNKNOWN],
                                           icon_size, icon_scale, NULL, 0, NULL);
    }

    /* We need a fallback */
    if (dev == NULL)
        dev = gtk_icon_theme_load_surface (netspeed->icon_theme,
                                           dev_type_icon [DEV_UNKNOWN],
                                           icon_size, icon_scale, NULL, 0, NULL);

    in_arrow = gtk_icon_theme_load_surface (netspeed->icon_theme,
                                            IN_ICON,
                                            16, icon_scale, NULL, 0, NULL);

    out_arrow = gtk_icon_theme_load_surface (netspeed->icon_theme,
                                             OUT_ICON,
                                             16, icon_scale, NULL, 0, NULL);

    /* Set the windowmanager icon for the applet */
    gtk_window_set_default_icon_name (LOGO_ICON);

    gtk_image_set_from_surface (GTK_IMAGE (netspeed->out_pix), out_arrow);
    gtk_image_set_from_surface (GTK_IMAGE (netspeed->in_pix), in_arrow);
    cairo_surface_destroy (in_arrow);
    cairo_surface_destroy (out_arrow);

    if (netspeed->devinfo && netspeed->devinfo->running) {
        gtk_widget_show (netspeed->in_box);
        gtk_widget_show (netspeed->out_box);
    } else {
        cairo_t *cr;
        cairo_surface_t *copy;
        gint down_coords;

        gtk_widget_hide (netspeed->in_box);
        gtk_widget_hide (netspeed->out_box);

        /* We're not allowed to modify "dev" */
        copy = cairo_surface_create_similar (dev,
                                             cairo_surface_get_content (dev),
                                             cairo_image_surface_get_width (dev) / icon_scale,
                                             cairo_image_surface_get_height (dev) / icon_scale);
        cr = cairo_create (copy);
        cairo_set_source_surface (cr, dev, 0, 0);
        cairo_paint (cr);

        down = gtk_icon_theme_load_surface (netspeed->icon_theme,
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

    if (netspeed->show_icon) {
        gtk_widget_show (netspeed->dev_pix);
        gtk_image_set_from_surface (GTK_IMAGE (netspeed->dev_pix), dev);
    } else {
        gtk_widget_hide (netspeed->dev_pix);
    }

    cairo_surface_destroy (dev);
}

/* Here some rearangement of the icons and the labels occurs
 * according to the panelsize and wether we show in and out
 * or just the sum
 */
static void
applet_change_size_or_orient (MatePanelApplet *applet,
                              int              arg1,
                              NetspeedApplet  *netspeed)
{
    int size;
    MatePanelAppletOrient orient;

    g_assert (netspeed);

    size = mate_panel_applet_get_size (applet);
    orient = mate_panel_applet_get_orient (applet);

    g_object_ref (netspeed->pix_box);
    g_object_ref (netspeed->in_pix);
    g_object_ref (netspeed->in_label);
    g_object_ref (netspeed->out_pix);
    g_object_ref (netspeed->out_label);
    g_object_ref (netspeed->sum_label);

    if (netspeed->in_box) {
        gtk_container_remove (GTK_CONTAINER (netspeed->in_box), netspeed->in_label);
        gtk_container_remove (GTK_CONTAINER (netspeed->in_box), netspeed->in_pix);
        gtk_widget_destroy (netspeed->in_box);
    }
    if (netspeed->out_box) {
        gtk_container_remove (GTK_CONTAINER (netspeed->out_box), netspeed->out_label);
        gtk_container_remove (GTK_CONTAINER (netspeed->out_box), netspeed->out_pix);
        gtk_widget_destroy (netspeed->out_box);
    }
    if (netspeed->sum_box) {
        gtk_container_remove (GTK_CONTAINER (netspeed->sum_box), netspeed->sum_label);
        gtk_widget_destroy (netspeed->sum_box);
    }
    if (netspeed->box) {
        gtk_container_remove (GTK_CONTAINER (netspeed->box), netspeed->pix_box);
        gtk_widget_destroy (netspeed->box);
    }

    if (orient == MATE_PANEL_APPLET_ORIENT_LEFT || orient == MATE_PANEL_APPLET_ORIENT_RIGHT) {
        netspeed->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        netspeed->speed_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        if (size > 64) {
            netspeed->sum_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
            netspeed->in_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
            netspeed->out_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
        } else {
            netspeed->sum_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
            netspeed->in_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
            netspeed->out_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        }
        netspeed->labels_dont_shrink = FALSE;
    } else {
        netspeed->in_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
        netspeed->out_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
        netspeed->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
        netspeed->sum_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        if (size < 48) {
            netspeed->speed_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
            netspeed->labels_dont_shrink = TRUE;
        } else {
            netspeed->speed_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
            netspeed->labels_dont_shrink = !netspeed->show_sum;
        }
    }

    gtk_box_pack_start (GTK_BOX (netspeed->in_box), netspeed->in_pix, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (netspeed->in_box), netspeed->in_label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (netspeed->out_box), netspeed->out_pix, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (netspeed->out_box), netspeed->out_label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (netspeed->sum_box), netspeed->sum_label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (netspeed->box), netspeed->pix_box, FALSE, FALSE, 0);

    g_object_unref (netspeed->pix_box);
    g_object_unref (netspeed->in_pix);
    g_object_unref (netspeed->in_label);
    g_object_unref (netspeed->out_pix);
    g_object_unref (netspeed->out_label);
    g_object_unref (netspeed->sum_label);

    if (netspeed->show_sum) {
        gtk_box_pack_start (GTK_BOX (netspeed->speed_box), netspeed->sum_box, TRUE, TRUE, 0);
    } else {
        gtk_box_pack_start (GTK_BOX (netspeed->speed_box), netspeed->in_box, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (netspeed->speed_box), netspeed->out_box, TRUE, TRUE, 0);
    }
    gtk_box_pack_start (GTK_BOX (netspeed->box), netspeed->speed_box, TRUE, TRUE, 0);

    gtk_widget_show_all (netspeed->box);
    if (!netspeed->show_icon) {
        gtk_widget_hide (netspeed->dev_pix);
    }
    gtk_container_add (GTK_CONTAINER (netspeed), netspeed->box);

    change_icons (netspeed);
}

/* Change visibility of signal quality icon for wireless devices
 */
static void
change_quality_icon (NetspeedApplet *netspeed)
{
    if (netspeed->devinfo->type == DEV_WIRELESS &&
        netspeed->devinfo->up && netspeed->show_quality_icon) {
        gtk_widget_show (netspeed->qual_pix);
    } else {
        gtk_widget_hide (netspeed->qual_pix);
    }
}

static void
update_quality_icon (NetspeedApplet *netspeed)
{
    if (!netspeed->show_quality_icon) {
        return;
    }

    unsigned int q;

    q = (netspeed->devinfo->qual);
    q /= 25;
    q = MIN (q, 3); /* q out of range would crash when accessing qual_surfaces[q] */
    gtk_image_set_from_surface (GTK_IMAGE (netspeed->qual_pix), netspeed->qual_surfaces[q]);
}

static void
init_quality_surfaces (NetspeedApplet *netspeed)
{
    int i;
    cairo_surface_t *surface;
    gint icon_scale;

    /* FIXME: Add larger icon files. */
    gint icon_size = 24;

    icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (netspeed));

    for (i = 0; i < 4; i++) {
        if (netspeed->qual_surfaces[i])
            cairo_surface_destroy (netspeed->qual_surfaces[i]);

        surface = gtk_icon_theme_load_surface (netspeed->icon_theme,
                                               wireless_quality_icon[i],
                                               icon_size, icon_scale, NULL, 0, NULL);

        if (surface) {
            cairo_t *cr;

            netspeed->qual_surfaces[i] = cairo_surface_create_similar (surface,
                                                                     cairo_surface_get_content (surface),
                                                                     cairo_image_surface_get_width (surface) / icon_scale,
                                                                     cairo_image_surface_get_height (surface) / icon_scale);

            cr = cairo_create (netspeed->qual_surfaces[i]);
            cairo_set_source_surface (cr, surface, 0, 0);
            cairo_paint (cr);
            cairo_surface_destroy (surface);
        }
        else {
            netspeed->qual_surfaces[i] = NULL;
        }
    }
}

static void
icon_theme_changed_cb (GtkIconTheme   *icon_theme,
                       NetspeedApplet *netspeed)
{
    init_quality_surfaces (netspeed);
    if (netspeed->devinfo->type == DEV_WIRELESS && netspeed->devinfo->up)
        update_quality_icon (netspeed);
    change_icons (netspeed);
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
redraw_graph (NetspeedApplet *netspeed,
              cairo_t        *cr)
{
    GtkWidget *da = GTK_WIDGET (netspeed->drawingarea);
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
    for (max_val = 1; max_val < netspeed->max_graph; max_val *= 2) ;

    /* calculate the polygons (GdkPoint[]) for the graphs */
    offset = 0;
    for (i = (netspeed->index_graph + 1) % GRAPH_VALUES; netspeed->in_graph[i] < 0; i = (i + 1) % GRAPH_VALUES)
        offset++;
    for (i = offset + 1; i < GRAPH_VALUES; i++)
    {
        int index = (netspeed->index_graph + i) % GRAPH_VALUES;
        out_points[i].x = in_points[i].x = ((w - 6) * i) / GRAPH_VALUES + 4;
        in_points[i].y = h - 6 - (int)((h - 8) * netspeed->in_graph[index] / max_val);
        out_points[i].y = h - 6 - (int)((h - 8) * netspeed->out_graph[index] / max_val);
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

    gdk_cairo_set_source_rgba (cr, &netspeed->in_color);
    for (i = offset; i < GRAPH_VALUES; i++) {
        cairo_line_to (cr, in_points[i].x, in_points[i].y);
    }
    cairo_stroke (cr);

    gdk_cairo_set_source_rgba (cr, &netspeed->out_color);
    for (i = offset; i < GRAPH_VALUES; i++) {
        cairo_line_to (cr, out_points[i].x, out_points[i].y);
    }
    cairo_stroke (cr);

    text = bps_to_string (max_val, netspeed->show_bits);
    add_markup_fgcolor (&text, "black");
    layout = gtk_widget_create_pango_layout (da, NULL);
    pango_layout_set_markup (layout, text, -1);
    g_free (text);
    gtk_render_layout (stylecontext, cr, 3, 2, layout);
    g_object_unref (G_OBJECT (layout));

    text = bps_to_string (0.0, netspeed->show_bits);
    add_markup_fgcolor (&text, "black");
    layout = gtk_widget_create_pango_layout (da, NULL);
    pango_layout_set_markup (layout, text, -1);
    pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
    g_free (text);
    gtk_render_layout (stylecontext, cr, 3, h - 4 - logical_rect.height, layout);
    g_object_unref (G_OBJECT (layout));
}

static gboolean
set_applet_devinfo (NetspeedApplet *netspeed,
                    const char     *iface)
{
    DevInfo *info;

    get_device_info (iface, &info);

    if (info->running) {
        free_device_info (netspeed->devinfo);
        netspeed->devinfo = info;
        netspeed->device_has_changed = TRUE;
        return TRUE;
    }

    free_device_info (info);
    return FALSE;
}

/* Find the first available device, that is running and != lo */
static void
search_for_up_if (NetspeedApplet *netspeed)
{
    const gchar *default_route;
    GList *devices, *tmp;

    default_route = get_default_route ();

    if (default_route != NULL) {
        if (set_applet_devinfo (netspeed, default_route))
            return;
    }

    devices = get_available_devices ();
    for (tmp = devices; tmp; tmp = g_list_next (tmp)) {
        if (is_dummy_device (tmp->data))
            continue;
        if (set_applet_devinfo (netspeed, tmp->data))
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
fill_details_dialog (NetspeedApplet *netspeed)
{
    char *text;
    char ipv4_text [INET_ADDRSTRLEN];

    if (netspeed->devinfo->ip) {
        format_ipv4 (netspeed->devinfo->ip, ipv4_text);
        text = ipv4_text;
    } else {
        text = _("none");
    }
    gtk_label_set_text (GTK_LABEL (netspeed->ip_text), text);

    if (netspeed->devinfo->netmask) {
        format_ipv4 (netspeed->devinfo->netmask, ipv4_text);
        text = ipv4_text;
    } else {
        text = _("none");
    }
    gtk_label_set_text (GTK_LABEL (netspeed->netmask_text), text);

    if (netspeed->devinfo->type != DEV_LO) {
        text = mac_addr_n2a (netspeed->devinfo->hwaddr);
        gtk_label_set_text (GTK_LABEL (netspeed->hwaddr_text), text);
        g_free (text);
    } else {
        gtk_label_set_text (GTK_LABEL (netspeed->hwaddr_text), _("none"));
    }

    if (netspeed->devinfo->ptpip) {
        format_ipv4 (netspeed->devinfo->ptpip, ipv4_text);
        text = ipv4_text;
    } else {
        text = _("none");
    }
    gtk_label_set_text (GTK_LABEL (netspeed->ptpip_text), text);

    /* check if we got an ipv6 address */
    GSList *ipv6_address_list = get_ip_address_list (netspeed->devinfo->name, FALSE);
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
            gtk_label_set_text (GTK_LABEL (netspeed->ipv6_text), string->str);
            gtk_widget_show (netspeed->ipv6_box);
        }
        g_string_free (string, TRUE);
        g_slist_free_full (ipv6_address_list, g_free);
    } else {
        gtk_widget_hide (netspeed->ipv6_box);
    }

    if (netspeed->devinfo->type == DEV_WIRELESS) {
        float quality;

        /* _maybe_ we can add the encrypted icon between the essid and the signal bar. */

        quality = netspeed->devinfo->qual / 100.0f;
        if (quality > 1.0)
            quality = 1.0;

        text = g_strdup_printf ("%d %%", netspeed->devinfo->qual);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (netspeed->signalbar), quality);
        gtk_progress_bar_set_text (GTK_PROGRESS_BAR (netspeed->signalbar), text);
        g_free (text);

        gtk_label_set_text (GTK_LABEL (netspeed->essid_text),
                            netspeed->devinfo->essid);

#ifdef HAVE_NL
        if (netspeed->devinfo->running) {
            text = mac_addr_n2a (netspeed->devinfo->station_mac_addr);
            gtk_label_set_text (GTK_LABEL (netspeed->station_text), text);
            g_free (text);
        } else {
            gtk_label_set_text (GTK_LABEL (netspeed->station_text), _("unknown"));
        }

        gtk_label_set_text (GTK_LABEL (netspeed->channel_text),
                            netspeed->devinfo->channel ? netspeed->devinfo->channel : _("unknown"));

        text = format_time (netspeed->devinfo->connected_time);
        gtk_label_set_text (GTK_LABEL (netspeed->connected_time_text),
                            netspeed->devinfo->connected_time > 0 ? text : _("na"));
        g_free (text);

        gtk_widget_show (netspeed->netlink_box);
#else
        gtk_widget_hide (netspeed->netlink_box);
#endif /* HAVE_NL */
        gtk_widget_show (netspeed->wireless_box);
    } else {
        gtk_widget_hide (netspeed->wireless_box);
    }
}

/* Here happens the really interesting stuff */
static void
update_applet (NetspeedApplet *netspeed)
{
    guint64 indiff, outdiff;
    double inrate, outrate;
    char *inbytes, *outbytes;
    int i;
    DevInfo *oldinfo;

    if (!netspeed) return;

    /* First we try to figure out if the device has changed */
    oldinfo = netspeed->devinfo;
    get_device_info (oldinfo->name, &netspeed->devinfo);
    if (compare_device_info (netspeed->devinfo, oldinfo))
        netspeed->device_has_changed = TRUE;
    free_device_info (oldinfo);

    /* If the device has changed, reintialize stuff */
    if (netspeed->device_has_changed) {
        change_icons (netspeed);
        change_quality_icon (netspeed);

        for (i = 0; i < OLD_VALUES; i++) {
            netspeed->in_old[i] = netspeed->devinfo->rx;
            netspeed->out_old[i] = netspeed->devinfo->tx;
        }

        for (i = 0; i < GRAPH_VALUES; i++) {
            netspeed->in_graph[i] = -1;
            netspeed->out_graph[i] = -1;
        }

        netspeed->max_graph = 0;
        netspeed->index_graph = 0;

        if (netspeed->details) {
            fill_details_dialog (netspeed);
        }

        netspeed->device_has_changed = FALSE;
    }

    /* create the strings for the labels and tooltips */
    if (netspeed->devinfo->running) {
        if (netspeed->devinfo->rx < netspeed->in_old[netspeed->index_old])
            indiff = 0;
        else
            indiff = netspeed->devinfo->rx - netspeed->in_old[netspeed->index_old];

        if (netspeed->devinfo->tx < netspeed->out_old[netspeed->index_old])
            outdiff = 0;
        else
            outdiff = netspeed->devinfo->tx - netspeed->out_old[netspeed->index_old];

        inrate = (double)indiff / OLD_VALUES_DBL;
        outrate = (double)outdiff / OLD_VALUES_DBL;

        netspeed->in_graph[netspeed->index_graph] = inrate;
        netspeed->out_graph[netspeed->index_graph] = outrate;
        netspeed->max_graph = MAX (inrate, netspeed->max_graph);
        netspeed->max_graph = MAX (outrate, netspeed->max_graph);

        format_transfer_rate (netspeed->devinfo->rx_rate, inrate, netspeed->show_bits);
        format_transfer_rate (netspeed->devinfo->tx_rate, outrate, netspeed->show_bits);
        format_transfer_rate (netspeed->devinfo->sum_rate, inrate + outrate, netspeed->show_bits);
    } else {
        netspeed->devinfo->rx_rate[0] = '\0';
        netspeed->devinfo->tx_rate[0] = '\0';
        netspeed->devinfo->sum_rate[0] = '\0';
        netspeed->in_graph[netspeed->index_graph] = 0;
        netspeed->out_graph[netspeed->index_graph] = 0;
    }

    if (netspeed->devinfo->type == DEV_WIRELESS) {
        if (netspeed->devinfo->up)
            update_quality_icon (netspeed);

        if (netspeed->signalbar) {
            float quality;
            char *text;

            quality = netspeed->devinfo->qual / 100.0f;
            if (quality > 1.0)
                quality = 1.0;

            text = g_strdup_printf ("%d %%", netspeed->devinfo->qual);
            gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (netspeed->signalbar), quality);
            gtk_progress_bar_set_text (GTK_PROGRESS_BAR (netspeed->signalbar), text);
            g_free (text);
        }
#ifdef HAVE_NL
        /* Refresh the value of Connected Time */
        if (netspeed->connected_time_text) {
            char *text;

            text = format_time (netspeed->devinfo->connected_time);
            gtk_label_set_text (GTK_LABEL (netspeed->connected_time_text),
                                netspeed->devinfo->connected_time > 0 ? text : _("na"));
            g_free (text);
        }
#endif
    }

    update_tooltip (netspeed);

    /* Refresh the text of the labels */
    if (netspeed->show_sum) {
        gtk_label_set_text (GTK_LABEL (netspeed->sum_label),
                            netspeed->devinfo->sum_rate);
    } else {
        gtk_label_set_text (GTK_LABEL (netspeed->in_label),
                            netspeed->devinfo->rx_rate);
        gtk_label_set_text (GTK_LABEL (netspeed->out_label),
                            netspeed->devinfo->tx_rate);
    }

    /* Refresh the values of the Infodialog */
    if (netspeed->inbytes_text) {
        if (netspeed->show_bits)
            inbytes = g_format_size_full (netspeed->devinfo->rx*8,
                                          G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_BITS);
        else
            inbytes = g_format_size_full (netspeed->devinfo->rx,
                                          G_FORMAT_SIZE_IEC_UNITS);

        gtk_label_set_text (GTK_LABEL (netspeed->inbytes_text), inbytes);
        g_free (inbytes);
    }
    if (netspeed->outbytes_text) {
        if (netspeed->show_bits)
            outbytes = g_format_size_full (netspeed->devinfo->tx*8,
                                           G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_BITS);
        else
            outbytes = g_format_size_full (netspeed->devinfo->tx,
                                           G_FORMAT_SIZE_IEC_UNITS);

        gtk_label_set_text (GTK_LABEL (netspeed->outbytes_text), outbytes);
        g_free (outbytes);
    }

    /* Redraw the graph of the Infodialog */
    if (netspeed->drawingarea)
        gtk_widget_queue_draw (GTK_WIDGET (netspeed->drawingarea));

    /* Save old values... */
    netspeed->in_old  [netspeed->index_old] = netspeed->devinfo->rx;
    netspeed->out_old [netspeed->index_old] = netspeed->devinfo->tx;
    netspeed->index_old = (netspeed->index_old + 1) % OLD_VALUES;

    /* Move the graphindex. Check if we can scale down again */
    netspeed->index_graph = (netspeed->index_graph + 1) % GRAPH_VALUES;
    if (netspeed->index_graph % 20 == 0) {
        double max = 0;

        for (i = 0; i < GRAPH_VALUES; i++) {
            max = MAX (max, netspeed->in_graph[i]);
            max = MAX (max, netspeed->out_graph[i]);
        }
        netspeed->max_graph = max;
    }

    /* Always follow the default route */
    if (netspeed->auto_change_device) {
        gboolean change_device_now = !netspeed->devinfo->running;

        if (!change_device_now) {
            const gchar *default_route;

            default_route = get_default_route ();
            change_device_now = (default_route != NULL &&
                                 strcmp (default_route, netspeed->devinfo->name));
        }
        if (change_device_now) {
            search_for_up_if (netspeed);
        }
    }
}

static gboolean
timeout_function (NetspeedApplet *netspeed)
{
    if (!netspeed)
        return FALSE;
    if (!netspeed->timeout_id)
        return FALSE;

    update_applet (netspeed);
    return TRUE;
}

/* Display a section of netspeed help
 */
void
netspeed_applet_display_help (GtkWidget   *dialog,
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
         NetspeedApplet *netspeed)
{
    netspeed_applet_display_help (GTK_WIDGET (netspeed), NULL);
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

static void
netspeed_applet_destory_preferences (GtkWidget *widget,
                                     gpointer   user_data)
{
	NetspeedApplet *netspeed;
	(void) widget;

	netspeed = NETSPEED_APPLET (user_data);
	netspeed->preferences = NULL;
}

/* Creates the settings dialog
 * After its been closed, take the new values and store
 * them in the gsettings database
 */
static void
settings_cb (GtkAction      *action,
             NetspeedApplet *netspeed)
{
    g_assert (netspeed);

    if (netspeed->preferences)
    {
        gtk_window_present (GTK_WINDOW (netspeed->preferences));
        return;
    }

    netspeed->preferences = netspeed_preferences_new (netspeed);
    g_signal_connect (netspeed->preferences, "destroy",
                      G_CALLBACK (netspeed_applet_destory_preferences), netspeed);
    gtk_widget_show_all (netspeed->preferences);
}

static gboolean
da_draw (GtkWidget      *widget,
         cairo_t        *cr,
         NetspeedApplet *netspeed)
{
    redraw_graph (netspeed, cr);

    return FALSE;
}

static void
incolor_changed_cb (GtkColorChooser *button,
                    NetspeedApplet  *netspeed)
{
    GdkRGBA color;
    gchar *string;

    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
    netspeed->in_color = color;

    string = gdk_rgba_to_string (&color);
    g_settings_set_string (netspeed->settings, "in-color", string);
    g_free (string);
}

static void
outcolor_changed_cb (GtkColorChooser *button,
                     NetspeedApplet  *netspeed)
{
    GdkRGBA color;
    gchar *string;

    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
    netspeed->out_color = color;

    string = gdk_rgba_to_string (&color);
    g_settings_set_string (netspeed->settings, "out-color", string);
    g_free (string);
}

/* Handle info dialog response event
 */
static void
info_response_cb (GtkDialog      *dialog,
                  gint            id,
                  NetspeedApplet *netspeed)
{

    if (id == GTK_RESPONSE_HELP) {
        netspeed_applet_display_help (GTK_WIDGET (dialog), "netspeed_applet-details");
        return;
    }

    gtk_widget_destroy (netspeed->details);

    netspeed->details       = NULL;
    netspeed->drawingarea   = NULL;
    netspeed->ip_text       = NULL;
    netspeed->netmask_text  = NULL;
    netspeed->ptpip_text    = NULL;
    netspeed->ipv6_text     = NULL;
    netspeed->hwaddr_text   = NULL;
    netspeed->inbytes_text  = NULL;
    netspeed->outbytes_text = NULL;
    netspeed->essid_text    = NULL;
    netspeed->signalbar     = NULL;
#ifdef HAVE_NL
    netspeed->station_text        = NULL;
    netspeed->channel_text        = NULL;
    netspeed->connected_time_text = NULL;
#endif /* HAVE_NL */
    netspeed->ipv6_box      = NULL;
    netspeed->netlink_box   = NULL;
    netspeed->wireless_box  = NULL;
}

/* Creates the details dialog
 */
static void
showinfo_cb (GtkAction      *action,
             NetspeedApplet *netspeed)
{
    GtkBuilder *builder;

    g_assert (netspeed);

    if (netspeed->details) {
        gtk_window_present (GTK_WINDOW (netspeed->details));
        return;
    }

    builder = gtk_builder_new_from_resource (NETSPEED_RESOURCE_PATH "netspeed-details.ui");

    netspeed->details       = GET_WIDGET ("dialog");
    netspeed->drawingarea   = GET_DRAWING_AREA ("drawingarea");

    netspeed->ip_text       = GET_WIDGET ("ip_text");
    netspeed->netmask_text  = GET_WIDGET ("netmask_text");
    netspeed->ptpip_text    = GET_WIDGET ("ptpip_text");
    netspeed->ipv6_text     = GET_WIDGET ("ipv6_text");
    netspeed->hwaddr_text   = GET_WIDGET ("hwaddr_text");
    netspeed->inbytes_text  = GET_WIDGET ("inbytes_text");
    netspeed->outbytes_text = GET_WIDGET ("outbytes_text");
    netspeed->essid_text    = GET_WIDGET ("essid_text");
    netspeed->signalbar     = GET_WIDGET ("signalbar");
#ifdef HAVE_NL
    netspeed->station_text  = GET_WIDGET ("station_text");
    netspeed->channel_text  = GET_WIDGET ("channel_text");

    netspeed->connected_time_text = GET_WIDGET ("connected_time_text");
#endif /* HAVE_NL */
    netspeed->ipv6_box      = GET_WIDGET ("ipv6_box");
    netspeed->netlink_box   = GET_WIDGET ("netlink_box");
    netspeed->wireless_box  = GET_WIDGET ("wireless_box");

    gtk_color_chooser_set_rgba (GET_COLOR_CHOOSER ("incolor_sel"),  &netspeed->in_color);
    gtk_color_chooser_set_rgba (GET_COLOR_CHOOSER ("outcolor_sel"),  &netspeed->out_color);

    fill_details_dialog (netspeed);

    gtk_builder_add_callback_symbols (builder,
                                      "on_drawingarea_draw", G_CALLBACK (da_draw),
                                      "on_incolor_sel_color_set", G_CALLBACK (incolor_changed_cb),
                                      "on_outcolor_sel_color_set", G_CALLBACK (outcolor_changed_cb),
                                      "on_dialog_response", G_CALLBACK (info_response_cb),
                                      NULL);
    gtk_builder_connect_signals (builder, netspeed);

    g_object_unref (builder);

    gtk_window_present (GTK_WINDOW (netspeed->details));
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
                        NetspeedApplet *netspeed)
{
    if (netspeed->labels_dont_shrink) {
        if (allocation->width <= netspeed->width)
            allocation->width = netspeed->width;
        else
            netspeed->width = allocation->width;
    }
}

static gboolean
netspeed_applet_button_press_event (GtkWidget      *widget,
                                    GdkEventButton *event)
{
    if (event->button == 1) {
        NetspeedApplet *netspeed = NETSPEED_APPLET (widget);
        GError *error = NULL;

        if (netspeed->connect_dialog) {
            gtk_window_present (GTK_WINDOW (netspeed->connect_dialog));
            return FALSE;
        }

        if (netspeed->up_cmd && netspeed->down_cmd) {
            char *question;
            int   response;

            if (netspeed->devinfo->up)
                question = g_strdup_printf (_("Do you want to disconnect %s now?"),
                                            netspeed->devinfo->name);
            else
                question = g_strdup_printf (_("Do you want to connect %s now?"),
                                            netspeed->devinfo->name);

            netspeed->connect_dialog =
                gtk_message_dialog_new (NULL,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                                        "%s", question);

            response = gtk_dialog_run (GTK_DIALOG (netspeed->connect_dialog));
            gtk_widget_destroy (netspeed->connect_dialog);
            netspeed->connect_dialog = NULL;
            g_free (question);

            if (response == GTK_RESPONSE_YES) {
                GtkWidget *dialog;
                char      *command;

                command = g_strdup_printf ("%s %s",
                                           netspeed->devinfo->up ? netspeed->down_cmd : netspeed->up_cmd,
                                           netspeed->devinfo->name);

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

    return GTK_WIDGET_CLASS (netspeed_applet_parent_class)->button_press_event (widget, event);
}

/* Frees the applet and all the data it contains
 * Removes the timeout_cb
 */
static void
netspeed_applet_finalize (GObject *object)
{
    NetspeedApplet *netspeed = NETSPEED_APPLET (object);

    if (netspeed->icon_theme != NULL) {
        g_signal_handlers_disconnect_by_func (netspeed->icon_theme,
                                              icon_theme_changed_cb,
                                              netspeed);
        netspeed->icon_theme = NULL;
    }

    if (netspeed->timeout_id > 0) {
        g_source_remove (netspeed->timeout_id);
        netspeed->timeout_id = 0;
    }

    g_clear_object (&netspeed->settings);

    g_clear_pointer (&netspeed->details, gtk_widget_destroy);
    g_clear_pointer (&netspeed->preferences, gtk_widget_destroy);

    g_free (netspeed->up_cmd);
    g_free (netspeed->down_cmd);

    /* Should never be NULL */
    free_device_info (netspeed->devinfo);
}

static void
update_tooltip (NetspeedApplet *netspeed)
{
    GString* tooltip;

    if (!netspeed->show_tooltip)
        return;

    tooltip = g_string_new ("");

    if (!netspeed->devinfo->running)
        g_string_printf (tooltip, _("%s is down"), netspeed->devinfo->name);
    else {
        char ipv4_text [INET_ADDRSTRLEN];
        char ipv6_text [INET6_ADDRSTRLEN];
        char *ip;

        if (netspeed->show_all_addresses) {
            format_ipv6 (netspeed->devinfo->ipv6, ipv6_text);
            if (netspeed->devinfo->ip) {
                format_ipv4 (netspeed->devinfo->ip, ipv4_text);
                if (strlen (ipv6_text) > 2) {
                    g_string_printf (tooltip,
                                     _("%s: %s and %s"),
                                     netspeed->devinfo->name,
                                     ipv4_text,
                                     ipv6_text);
                } else {
                    g_string_printf (tooltip,
                                     _("%s: %s"),
                                     netspeed->devinfo->name,
                                     ipv4_text);
                }
            } else {
                if (strlen (ipv6_text) > 2) {
                    g_string_printf (tooltip,
                                     _("%s: %s"),
                                     netspeed->devinfo->name,
                                     ipv6_text);
                } else {
                    g_string_printf (tooltip,
                                     _("%s: has no ip"),
                                     netspeed->devinfo->name);
                }
            }
        } else {
            if (netspeed->devinfo->ip) {
                format_ipv4 (netspeed->devinfo->ip, ipv4_text);
                ip = ipv4_text;
            } else {
                format_ipv6 (netspeed->devinfo->ipv6, ipv6_text);
                if (strlen (ipv6_text) > 2) {
                    ip = ipv6_text;
                } else {
                    ip = _("has no ip");
                }
            }
            g_string_printf (tooltip,
                             _("%s: %s"),
                             netspeed->devinfo->name,
                             ip);
        }

        if (netspeed->show_sum) {
            g_string_append_printf (tooltip,
                                    _("\nin: %s out: %s"),
                                    netspeed->devinfo->rx_rate,
                                    netspeed->devinfo->tx_rate);
        } else {
            g_string_append_printf (tooltip,
                                    _("\nsum: %s"),
                                    netspeed->devinfo->sum_rate);
        }

#ifdef HAVE_NL
        if (netspeed->devinfo->type == DEV_WIRELESS)
            g_string_append_printf (tooltip,
                                    _("\nESSID: %s\nRSSI: %d dBm\nRX Bitrate: %s\nTX Bitrate: %s"),
                                    netspeed->devinfo->essid ? netspeed->devinfo->essid : _("unknown"),
                                    netspeed->devinfo->rssi,
                                    netspeed->devinfo->rx_bitrate,
                                    netspeed->devinfo->tx_bitrate);
#endif /* HAVE_NL */
#ifdef HAVE_IW
        if (netspeed->devinfo->type == DEV_WIRELESS)
            g_string_append_printf (tooltip,
                                    _("\nESSID: %s\nStrength: %d %%"),
                                    netspeed->devinfo->essid ? netspeed->devinfo->essid : _("unknown"),
                                    netspeed->devinfo->qual);
#endif /* HAVE_IW */
    }

    gtk_widget_set_tooltip_text (GTK_WIDGET (netspeed), tooltip->str);
    gtk_widget_trigger_tooltip_query (GTK_WIDGET (netspeed));
    g_string_free (tooltip, TRUE);
}


static gboolean
netspeed_applet_enter_notify_event (GtkWidget        *widget,
                                    GdkEventCrossing *event)
{
    NetspeedApplet *netspeed = NETSPEED_APPLET (widget);

    netspeed->show_tooltip = TRUE;
    update_tooltip (netspeed);
    return TRUE;
}

static gboolean
netspeed_applet_leave_notify_event (GtkWidget        *widget,
                                    GdkEventCrossing *event)
{
    NetspeedApplet *netspeed = NETSPEED_APPLET (widget);

    netspeed->show_tooltip = FALSE;
    return TRUE;
}

static void
netspeed_applet_class_init (NetspeedAppletClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->finalize = netspeed_applet_finalize;

    widget_class->button_press_event = netspeed_applet_button_press_event;
    widget_class->leave_notify_event = netspeed_applet_leave_notify_event;
    widget_class->enter_notify_event = netspeed_applet_enter_notify_event;
}

static void
netspeed_applet_init (NetspeedApplet *netspeed)
{
}

static void
changeicon_settings_changed (GSettings      *settings,
                             const gchar    *key,
                             NetspeedApplet *netspeed)
{
    netspeed->change_icon = g_settings_get_boolean (settings, key);
    change_icons (netspeed);
}

static void
showalladdresses_settings_changed (GSettings      *settings,
                                   const gchar    *key,
                                   NetspeedApplet *netspeed)
{
    netspeed->show_all_addresses = g_settings_get_boolean (settings, key);
}

static void
showsum_settings_changed (GSettings      *settings,
                          const gchar    *key,
                          NetspeedApplet *netspeed)
{
    netspeed->show_sum = g_settings_get_boolean (settings, key);
    applet_change_size_or_orient (MATE_PANEL_APPLET (netspeed), -1, netspeed);
    change_icons (netspeed);
}

static void
showbits_settings_changed (GSettings      *settings,
                           const gchar    *key,
                           NetspeedApplet *netspeed)
{
    netspeed->show_bits = g_settings_get_boolean (settings, key);
}

static void
showicon_settings_changed (GSettings      *settings,
                           const gchar    *key,
                           NetspeedApplet *netspeed)
{
    netspeed->show_icon = g_settings_get_boolean (settings, key);
    change_icons (netspeed);
}

static void
showqualityicon_settings_changed (GSettings      *settings,
                                  const gchar    *key,
                                  NetspeedApplet *netspeed)
{
    netspeed->show_quality_icon = g_settings_get_boolean (settings, key);
    change_quality_icon (netspeed);
}

static void
auto_change_device_settings_changed (GSettings      *settings,
                                     const gchar    *key,
                                     NetspeedApplet *netspeed)
{
    netspeed->auto_change_device = g_settings_get_boolean (settings, key);
    netspeed->device_has_changed = TRUE;
    update_applet (netspeed);
}

static void
device_settings_changed (GSettings      *settings,
                         const gchar    *key,
                         NetspeedApplet *netspeed)
{
    char *davice;

    davice = g_settings_get_string (settings, key);
    set_applet_devinfo (netspeed, davice);
    g_free (davice);
}

/* The "main" function of the applet
 */
static gboolean
netspeed_applet_factory (MatePanelApplet *applet,
                         const gchar     *iid,
                         gpointer         data)
{
    NetspeedApplet *netspeed;
    int i;
    GtkWidget *spacer, *spacer_box;
    GtkActionGroup *action_group;

    if (strcmp (iid, "NetspeedApplet"))
        return FALSE;

    glibtop_init ();
    g_set_application_name (_("MATE Netspeed"));

    netspeed = NETSPEED_APPLET (applet);
    netspeed->icon_theme = gtk_icon_theme_get_default ();

    /* Set the default colors of the graph
    */
    netspeed->in_color.red = 0xdf00;
    netspeed->in_color.green = 0x2800;
    netspeed->in_color.blue = 0x4700;
    netspeed->out_color.red = 0x3700;
    netspeed->out_color.green = 0x2800;
    netspeed->out_color.blue = 0xdf00;

    for (i = 0; i < GRAPH_VALUES; i++)
    {
        netspeed->in_graph[i] = -1;
        netspeed->out_graph[i] = -1;
    }

    netspeed->settings = mate_panel_applet_settings_new (applet, "org.mate.panel.applet.netspeed");

    /* Get stored settings from gsettings
     */
    netspeed->show_all_addresses = g_settings_get_boolean (netspeed->settings, "show-all-addresses");
    netspeed->show_sum = g_settings_get_boolean (netspeed->settings, "show-sum");
    netspeed->show_bits = g_settings_get_boolean (netspeed->settings, "show-bits");
    netspeed->show_icon = g_settings_get_boolean (netspeed->settings, "show-icon");
    netspeed->show_quality_icon = g_settings_get_boolean (netspeed->settings, "show-quality-icon");
    netspeed->change_icon = g_settings_get_boolean (netspeed->settings, "change-icon");
    netspeed->auto_change_device = g_settings_get_boolean (netspeed->settings, "auto-change-device");

    char *tmp = NULL;
    tmp = g_settings_get_string (netspeed->settings, "device");
    if (tmp && strcmp (tmp, "")) {
        get_device_info (tmp, &netspeed->devinfo);
        g_free (tmp);
    }

    tmp = g_settings_get_string (netspeed->settings, "up-command");
    if (tmp && strcmp (tmp, "")) {
        netspeed->up_cmd = g_strdup (tmp);
        g_free (tmp);
    }

    tmp = g_settings_get_string (netspeed->settings, "down-command");
    if (tmp && strcmp (tmp, "")) {
        netspeed->down_cmd = g_strdup (tmp);
        g_free (tmp);
    }

    tmp = g_settings_get_string (netspeed->settings, "in-color");
    if (tmp) {
        gdk_rgba_parse (&netspeed->in_color, tmp);
        g_free (tmp);
    }

    tmp = g_settings_get_string (netspeed->settings, "out-color");
    if (tmp) {
        gdk_rgba_parse (&netspeed->out_color, tmp);
        g_free (tmp);
    }

    if (!netspeed->devinfo) {
        GList *ptr, *devices = get_available_devices ();
        ptr = devices;
        while (ptr) {
            if (!g_str_equal (ptr->data, "lo")) {
                get_device_info (ptr->data, &netspeed->devinfo);
                break;
            }
            ptr = g_list_next (ptr);
        }
        free_devices_list (devices);
    }
    if (!netspeed->devinfo)
        get_device_info ("lo", &netspeed->devinfo);

    netspeed->device_has_changed = TRUE;

    netspeed->in_label = gtk_label_new ("");
    netspeed->out_label = gtk_label_new ("");
    netspeed->sum_label = gtk_label_new ("");

    netspeed->in_pix = gtk_image_new ();
    netspeed->out_pix = gtk_image_new ();
    netspeed->dev_pix = gtk_image_new ();
    netspeed->qual_pix = gtk_image_new ();

    netspeed->pix_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    spacer = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (netspeed->pix_box), spacer, TRUE, TRUE, 0);
    spacer = gtk_label_new ("");
    gtk_box_pack_end (GTK_BOX (netspeed->pix_box), spacer, TRUE, TRUE, 0);

    spacer_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start (GTK_BOX (netspeed->pix_box), spacer_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (spacer_box), netspeed->qual_pix, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (spacer_box), netspeed->dev_pix, FALSE, FALSE, 0);

    init_quality_surfaces (netspeed);

    applet_change_size_or_orient (applet, -1, netspeed);
    gtk_widget_show_all (GTK_WIDGET (applet));
    update_applet (netspeed);

    mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);

    netspeed->timeout_id = g_timeout_add (REFRESH_TIME,
                                         (GSourceFunc)timeout_function,
                                         netspeed);

    g_signal_connect (applet, "change_size",
                      G_CALLBACK (applet_change_size_or_orient),
                      netspeed);

    g_signal_connect (netspeed->icon_theme, "changed",
                      G_CALLBACK (icon_theme_changed_cb),
                      netspeed);

    g_signal_connect (applet, "change_orient",
                      G_CALLBACK (applet_change_size_or_orient),
                      netspeed);

    g_signal_connect (netspeed->in_label, "size_allocate",
                      G_CALLBACK (label_size_allocate_cb),
                      netspeed);

    g_signal_connect (netspeed->out_label, "size_allocate",
                      G_CALLBACK (label_size_allocate_cb),
                      netspeed);

    g_signal_connect (netspeed->sum_label, "size_allocate",
                      G_CALLBACK (label_size_allocate_cb),
                      netspeed);

    g_signal_connect (netspeed->settings, "changed::auto-change-device",
                      G_CALLBACK (auto_change_device_settings_changed),
                      netspeed);

    g_signal_connect (netspeed->settings, "changed::device",
                      G_CALLBACK (device_settings_changed),
                      netspeed);

    g_signal_connect (netspeed->settings, "changed::show-all-addresses",
                      G_CALLBACK (showalladdresses_settings_changed),
                      netspeed);

    g_signal_connect (netspeed->settings, "changed::show-sum",
                      G_CALLBACK (showsum_settings_changed),
                      netspeed);

    g_signal_connect (netspeed->settings, "changed::show-bits",
                      G_CALLBACK (showbits_settings_changed),
                      netspeed);

    g_signal_connect (netspeed->settings, "changed::change-icon",
                      G_CALLBACK (changeicon_settings_changed),
                      netspeed);

    g_signal_connect (netspeed->settings, "changed::show-icon",
                      G_CALLBACK (showicon_settings_changed),
                      netspeed);

    g_signal_connect (netspeed->settings, "changed::show-quality-icon",
                      G_CALLBACK (showqualityicon_settings_changed),
                      netspeed);

    action_group = gtk_action_group_new ("Netspeed Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group,
                                  netspeed_applet_menu_actions,
                                  G_N_ELEMENTS (netspeed_applet_menu_actions),
                                  netspeed);

    mate_panel_applet_setup_menu_from_resource (applet,
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
