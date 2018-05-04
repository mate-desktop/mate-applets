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
 * Authors : Carlos García Campos <carlosgc@gnome.org>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gio.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#include "cpufreq-applet.h"
#include "cpufreq-prefs.h"
#include "cpufreq-popup.h"
#include "cpufreq-monitor.h"
#include "cpufreq-monitor-factory.h"
#include "cpufreq-utils.h"

struct _CPUFreqApplet {
        MatePanelApplet       base;

        /* Visibility */
	CPUFreqShowMode   show_mode;
	CPUFreqShowTextMode show_text_mode;
        gboolean          show_freq;
        gboolean          show_perc;
        gboolean          show_unit;
        gboolean          show_icon;

        CPUFreqMonitor   *monitor;

        MatePanelAppletOrient orient;
        gint              size;

        GtkWidget        *label;
        GtkWidget        *unit_label;
        GtkWidget        *icon;
        GtkWidget        *box;
	GtkWidget        *labels_box;
        GtkWidget        *container;
        cairo_surface_t  *surfaces[5];

	gint              max_label_width;
	gint              max_perc_width;
	gint              max_unit_width;

	gboolean          need_refresh;

        CPUFreqPrefs     *prefs;
        CPUFreqPopup     *popup;
};

struct _CPUFreqAppletClass {
        MatePanelAppletClass parent_class;
};

static void     cpufreq_applet_init              (CPUFreqApplet      *applet);
static void     cpufreq_applet_class_init        (CPUFreqAppletClass *klass);

static void     cpufreq_applet_preferences_cb    (GtkAction          *action,
                                                  CPUFreqApplet      *applet);
static void     cpufreq_applet_help_cb           (GtkAction          *action,
                                                  CPUFreqApplet      *applet);
static void     cpufreq_applet_about_cb          (GtkAction          *action,
                                                  CPUFreqApplet      *applet);

static void     cpufreq_applet_pixmap_set_image  (CPUFreqApplet      *applet,
                                                  gint                perc);

static void     cpufreq_applet_setup             (CPUFreqApplet      *applet);
static void     cpufreq_applet_update            (CPUFreqApplet      *applet,
                                                  CPUFreqMonitor     *monitor);
static void     cpufreq_applet_refresh           (CPUFreqApplet      *applet);

static void     cpufreq_applet_dispose           (GObject            *widget);
static gboolean cpufreq_applet_button_press      (GtkWidget          *widget,
                                                  GdkEventButton     *event);
static gboolean cpufreq_applet_key_press         (GtkWidget          *widget,
                                                  GdkEventKey        *event);
static void     cpufreq_applet_size_allocate     (GtkWidget          *widget,
                                                  GtkAllocation      *allocation);
static void     cpufreq_applet_get_preferred_width   (GtkWidget *widget,
                                                      gint *minimum_width,
                                                      gint *natural_width);
static void     cpufreq_applet_change_orient     (MatePanelApplet        *pa,
                                                  MatePanelAppletOrient   orient);
static void     cpufreq_applet_style_updated     (GtkWidget *widget);
static gboolean cpufreq_applet_factory           (CPUFreqApplet      *applet,
                                                  const gchar        *iid,
                                                  gpointer            gdata);

static const gchar* const cpufreq_icons[] = {
	MATE_PIXMAPSDIR "/mate-cpufreq-applet/cpufreq-25.png",
	MATE_PIXMAPSDIR "/mate-cpufreq-applet/cpufreq-50.png",
	MATE_PIXMAPSDIR "/mate-cpufreq-applet/cpufreq-75.png",
	MATE_PIXMAPSDIR "/mate-cpufreq-applet/cpufreq-100.png",
	MATE_PIXMAPSDIR "/mate-cpufreq-applet/cpufreq-na.png",
	NULL
};

static const GtkActionEntry cpufreq_applet_menu_actions[] = {
	{ "CPUFreqAppletPreferences", "document-properties", N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (cpufreq_applet_preferences_cb) },
	{ "CPUFreqAppletHelp", "help-browser", N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (cpufreq_applet_help_cb) },
	{ "CPUFreqAppletAbout", "help-about", N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (cpufreq_applet_about_cb) }
};

G_DEFINE_TYPE (CPUFreqApplet, cpufreq_applet, PANEL_TYPE_APPLET)

/* Enum Types */
GType
cpufreq_applet_show_mode_get_type (void)
{
        static GType etype = 0;

        if (etype == 0) {
                static const GEnumValue values[] = {
                        { CPUFREQ_MODE_GRAPHIC, "CPUFREQ_MODE_GRAPHIC", "mode-graphic" },
                        { CPUFREQ_MODE_TEXT,    "CPUFREQ_MODE_TEXT",    "mode-text" },
                        { CPUFREQ_MODE_BOTH,    "CPUFREQ_MODE_BOTH",    "mode-both" },
                        { 0, NULL, NULL }
                };

                etype = g_enum_register_static ("CPUFreqShowMode", values);
        }

        return etype;
}

GType
cpufreq_applet_show_text_mode_get_type (void)
{
        static GType etype = 0;

        if (etype == 0) {
                static const GEnumValue values[] = {
                        { CPUFREQ_MODE_TEXT_FREQUENCY,      "CPUFREQ_MODE_TEXT_FREQUENCY",      "mode-text-frequency" },
                        { CPUFREQ_MODE_TEXT_FREQUENCY_UNIT, "CPUFREQ_MODE_TEXT_FREQUENCY_UNIT", "mode-text-frequency-unit" },
                        { CPUFREQ_MODE_TEXT_PERCENTAGE,     "CPUFREQ_MODE_TEXT_PERCENTAGE",     "mode-text-percentage" },
                        { 0, NULL, NULL }
                };

                etype = g_enum_register_static ("CPUFreqShowTextMode", values);
        }

        return etype;
}

static void
cpufreq_applet_init (CPUFreqApplet *applet)
{
        applet->prefs = NULL;
        applet->popup = NULL;
        applet->monitor = NULL;

        applet->label = gtk_label_new (NULL);
        applet->unit_label = gtk_label_new (NULL);
        applet->icon = gtk_image_new ();
        applet->box = NULL;

	applet->show_mode = CPUFREQ_MODE_BOTH;
	applet->show_text_mode = CPUFREQ_MODE_TEXT_FREQUENCY_UNIT;

	applet->need_refresh = TRUE;

        mate_panel_applet_set_flags (MATE_PANEL_APPLET (applet), MATE_PANEL_APPLET_EXPAND_MINOR);
	mate_panel_applet_set_background_widget (MATE_PANEL_APPLET (applet), GTK_WIDGET (applet));

        applet->size = mate_panel_applet_get_size (MATE_PANEL_APPLET (applet));
        applet->orient = mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet));

	switch (applet->orient) {
	case MATE_PANEL_APPLET_ORIENT_LEFT:
	case MATE_PANEL_APPLET_ORIENT_RIGHT:
		applet->container = gtk_alignment_new (0.5, 0.5, 0, 0);
		break;
	case MATE_PANEL_APPLET_ORIENT_UP:
	case MATE_PANEL_APPLET_ORIENT_DOWN:
		applet->container = gtk_alignment_new (0, 0.5, 0, 0);
		break;
	}

	gtk_container_add (GTK_CONTAINER (applet), applet->container);
	gtk_widget_show (applet->container);
}

static void
cpufreq_applet_class_init (CPUFreqAppletClass *klass)
{
        MatePanelAppletClass *applet_class = MATE_PANEL_APPLET_CLASS (klass);
        GObjectClass     *gobject_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);

        gobject_class->dispose = cpufreq_applet_dispose;

        widget_class->size_allocate = cpufreq_applet_size_allocate;
        widget_class->style_updated = cpufreq_applet_style_updated;
	widget_class->get_preferred_width = cpufreq_applet_get_preferred_width;
        widget_class->button_press_event = cpufreq_applet_button_press;
        widget_class->key_press_event = cpufreq_applet_key_press;

        applet_class->change_orient = cpufreq_applet_change_orient;
}

static void
cpufreq_applet_dispose (GObject *widget)
{
        CPUFreqApplet *applet;
        gint           i;

        applet = CPUFREQ_APPLET (widget);

        if (applet->monitor) {
                g_object_unref (G_OBJECT (applet->monitor));
                applet->monitor = NULL;
        }

        for (i = 0; i <= 3; i++) {
                if (applet->surfaces[i]) {
                        cairo_surface_destroy (applet->surfaces[i]);
                        applet->surfaces[i] = NULL;
                }
        }

        if (applet->prefs) {
                g_object_unref (applet->prefs);
                applet->prefs = NULL;
        }

        if (applet->popup) {
                g_object_unref (applet->popup);
                applet->popup = NULL;
        }

        G_OBJECT_CLASS (cpufreq_applet_parent_class)->dispose (widget);
}

static void
cpufreq_applet_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
        CPUFreqApplet *applet;
        gint           size = 0;

        applet = CPUFREQ_APPLET (widget);

        switch (applet->orient) {
        case MATE_PANEL_APPLET_ORIENT_LEFT:
        case MATE_PANEL_APPLET_ORIENT_RIGHT:
                size = allocation->width;
                break;
        case MATE_PANEL_APPLET_ORIENT_UP:
        case MATE_PANEL_APPLET_ORIENT_DOWN:
                size = allocation->height;
                break;
        }

        if (size != applet->size) {
                applet->size = size;
                cpufreq_applet_refresh (applet);
        }

        GTK_WIDGET_CLASS (cpufreq_applet_parent_class)->size_allocate (widget, allocation);
}

static gint
get_max_text_width (GtkWidget *widget,
                    const char *text)
{
	PangoContext *context;
	PangoLayout *layout;
	PangoRectangle logical_rect;

	context = gtk_widget_get_pango_context (widget);
	layout = pango_layout_new (context);
	pango_layout_set_text (layout, text, -1);
	pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

	g_object_unref (layout);

	return logical_rect.width;
}

static gint
cpufreq_applet_get_max_label_width (CPUFreqApplet *applet)
{
	GList *available_freqs;
	gint   width = 0;

	if (applet->max_label_width > 0)
		return applet->max_label_width;

	if (!CPUFREQ_IS_MONITOR (applet->monitor))
		return 0;

	available_freqs = cpufreq_monitor_get_available_frequencies (applet->monitor);
	while (available_freqs) {
		gint           label_width;
		const gchar   *text;
		gchar         *freq_text;
		gint           freq;

		text = (const gchar *) available_freqs->data;
		freq = atoi (text);

		freq_text = cpufreq_utils_get_frequency_label (freq);
		label_width = get_max_text_width (applet->label, freq_text);
		width = MAX (width, label_width); 

		g_free (freq_text);

		available_freqs = g_list_next (available_freqs);
	}

	applet->max_label_width = width;

	return width;
}

static gint
cpufreq_applet_get_max_perc_width (CPUFreqApplet *applet)
{
	if (applet->max_perc_width > 0)
		return applet->max_perc_width;

	applet->max_perc_width = get_max_text_width (applet->label, "100%");
	return applet->max_perc_width;
}

static gint
cpufreq_applet_get_max_unit_width (CPUFreqApplet *applet)
{
	if (applet->max_unit_width > 0)
		return applet->max_unit_width;

	applet->max_unit_width = MAX (get_max_text_width (applet->unit_label, "GHz"),
	                              get_max_text_width (applet->unit_label, "MHz"));
	return applet->max_unit_width;
}

static void
cpufreq_applet_get_preferred_width (GtkWidget *widget, gint *minimum_width, gint *natural_width)
{
	CPUFreqApplet *applet;
	gint           labels_width = 0;
	gint           width;
	gint           scale;

	applet = CPUFREQ_APPLET (widget);
	scale = gtk_widget_get_scale_factor (widget);

	if (applet->orient == MATE_PANEL_APPLET_ORIENT_LEFT ||
	    applet->orient == MATE_PANEL_APPLET_ORIENT_RIGHT)
		return;


 	if (applet->show_freq) {
		labels_width += cpufreq_applet_get_max_label_width (applet) + 2;
 	}
 
 	if (applet->show_perc) {
		labels_width += cpufreq_applet_get_max_perc_width (applet);
 	}
 
 	if (applet->show_unit) {
		labels_width += cpufreq_applet_get_max_unit_width (applet);
	}

	if (applet->show_icon) {
		gint icon_width;

		gtk_widget_get_preferred_width (applet->icon, &icon_width, NULL);
		width = (labels_width + icon_width + 2);
	} else {
		width = labels_width;
	}

	*minimum_width = *natural_width = width / scale;
}

static void
cpufreq_applet_popup_position_menu (GtkMenu  *menu,
                                    int      *x,
                                    int      *y,
                                    gboolean *push_in,
                                    gpointer  gdata)
{
        GtkWidget      *widget;
        GtkRequisition  requisition;
        GtkAllocation   allocation;
        gint            menu_xpos;
        gint            menu_ypos;

        widget = GTK_WIDGET (gdata);

        gtk_widget_get_preferred_size (GTK_WIDGET (menu), &requisition, NULL);

        gdk_window_get_origin (gtk_widget_get_window (widget), &menu_xpos, &menu_ypos);

        gtk_widget_get_allocation (widget, &allocation);

        menu_xpos += allocation.x;
        menu_ypos += allocation.y;

        switch (mate_panel_applet_get_orient (MATE_PANEL_APPLET (widget))) {
        case MATE_PANEL_APPLET_ORIENT_DOWN:
        case MATE_PANEL_APPLET_ORIENT_UP:
                if (menu_ypos > HeightOfScreen (gdk_x11_screen_get_xscreen (gtk_widget_get_screen (widget))) / 2)
                        menu_ypos -= requisition.height;
                else
                        menu_ypos += allocation.height;
                break;
        case MATE_PANEL_APPLET_ORIENT_RIGHT:
        case MATE_PANEL_APPLET_ORIENT_LEFT:
                if (menu_xpos > WidthOfScreen (gdk_x11_screen_get_xscreen (gtk_widget_get_screen (widget))) / 2)
                        menu_xpos -= requisition.width;
                else
                        menu_xpos += allocation.width;
                break;
        default:
                g_assert_not_reached ();
        }

        *x = menu_xpos;
        *y = menu_ypos;
        if (push_in) *push_in = FALSE;  /*fix bottom panel menu rendering in gtk3*/
}

static void
cpufreq_applet_menu_popup (CPUFreqApplet *applet,
                           guint32        time)
{
        GtkWidget *menu;

        if (!cpufreq_utils_selector_is_available ())
                return;

        if (!applet->popup) {
                applet->popup = cpufreq_popup_new ();
                cpufreq_popup_set_monitor (applet->popup, applet->monitor);
		cpufreq_popup_set_parent (applet->popup, GTK_WIDGET (applet));
        }

        menu = cpufreq_popup_get_menu (applet->popup);

        if (!menu)
                return;
                
        /*Set up theme and transparency support*/
        GtkWidget *toplevel = gtk_widget_get_toplevel (menu);
        /* Fix any failures of compiz/other wm's to communicate with gtk for transparency */
        GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(toplevel));
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        gtk_widget_set_visual(GTK_WIDGET(toplevel), visual);
        /* Set menu and it's toplevel window to follow panel theme */
        GtkStyleContext *context;
        context = gtk_widget_get_style_context (GTK_WIDGET(toplevel));
        gtk_style_context_add_class(context,"gnome-panel-menu-bar");
        gtk_style_context_add_class(context,"mate-panel-menu-bar");

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
                        cpufreq_applet_popup_position_menu,
                        (gpointer) applet,
                        1, time);
}

static gboolean
cpufreq_applet_button_press (GtkWidget *widget, GdkEventButton *event)
{
        CPUFreqApplet *applet;

        applet = CPUFREQ_APPLET (widget);

        if (event->button == 2)
                return FALSE;

        if (event->button == 1 &&
            event->type != GDK_2BUTTON_PRESS &&
            event->type != GDK_3BUTTON_PRESS) {
                cpufreq_applet_menu_popup (applet, event->time);

                return TRUE;
        }

        return GTK_WIDGET_CLASS (cpufreq_applet_parent_class)->button_press_event (widget, event);
}

static gboolean
cpufreq_applet_key_press (GtkWidget *widget, GdkEventKey *event)
{
        CPUFreqApplet *applet;

        applet = CPUFREQ_APPLET (widget);

        switch (event->keyval) {
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
        case GDK_KEY_3270_Enter:
        case GDK_KEY_Return:
        case GDK_KEY_space:
        case GDK_KEY_KP_Space:
                cpufreq_applet_menu_popup (applet, event->time);

                return TRUE;
        default:
                break;
        }

        return FALSE;
}

static void
cpufreq_applet_change_orient (MatePanelApplet *pa, MatePanelAppletOrient orient)
{
        CPUFreqApplet *applet;
        GtkAllocation  allocation;
        gint           size;

        applet = CPUFREQ_APPLET (pa);

        applet->orient = orient;

        gtk_widget_get_allocation (GTK_WIDGET (applet), &allocation);

        if ((orient == MATE_PANEL_APPLET_ORIENT_LEFT) ||
            (orient == MATE_PANEL_APPLET_ORIENT_RIGHT)) {
                size = allocation.width;
		gtk_alignment_set (GTK_ALIGNMENT (applet->container),
				   0.5, 0.5, 0, 0);
        } else {
                size = allocation.height;
		gtk_alignment_set (GTK_ALIGNMENT (applet->container),
				   0, 0.5, 0, 0);
        }

        if (size != applet->size) {
                applet->size = size;
                cpufreq_applet_refresh (applet);
        }
}

static void
cpufreq_applet_style_updated (GtkWidget *widget)
{
        CPUFreqApplet *applet;

        applet = CPUFREQ_APPLET (widget);

        applet->max_label_width = 0;
        applet->max_unit_width = 0;
        applet->max_perc_width = 0;

        cpufreq_applet_refresh (applet);
}

static void
cpufreq_applet_preferences_cb (GtkAction     *action,
                               CPUFreqApplet *applet)
{
        cpufreq_preferences_dialog_run (applet->prefs,
                                        gtk_widget_get_screen (GTK_WIDGET (applet)));
}

static void
cpufreq_applet_help_cb (GtkAction     *action,
                        CPUFreqApplet *applet)
{
        GError *error = NULL;

	gtk_show_uri_on_window (NULL,
	                        "help:mate-cpufreq-applet",
	                        gtk_get_current_event_time (),
	                        &error);

        if (error) {
                cpufreq_utils_display_error (_("Could not open help document"),
                                             error->message);
                g_error_free (error);
        }
}

static void
cpufreq_applet_about_cb (GtkAction     *action,
                         CPUFreqApplet *applet)
{
        char copyright[] = \
                "Copyright \xc2\xa9 2012-2018 MATE developers\n"
                "Copyright \xC2\xA9 2004 Carlos Garcia Campos";

        static const gchar *const authors[] = {
                "Carlos Garcia Campos <carlosgc@gnome.org>",
                NULL
        };
        static const gchar *const documenters[] = {
                "Carlos Garcia Campos <carlosgc@gnome.org>",
                "Davyd Madeley <davyd@madeley.id.au>",
                NULL
        };
        static const gchar *const artists[] = {
                "Pablo Arroyo Loma <zzioma@yahoo.es>",
                NULL
        };

        gtk_show_about_dialog (NULL,
                               "version",       VERSION,
                               "copyright",     copyright,
                               "comments",      _("This utility shows the current CPU "
                                                  "Frequency Scaling."),
                               "authors",       authors,
                               "documenters",   documenters,
                               "artists",       artists,
                               "translator-credits",    _("translator-credits"),
			       "logo-icon-name",        "mate-cpu-frequency-applet",
                               NULL);
}

static void
cpufreq_applet_pixmap_set_image (CPUFreqApplet *applet, gint perc)
{
        gint image;
        gint scale;
        gint size = 24; /* FIXME */

        /* 0-29   -> 25%
         * 30-69  -> 50%
         * 70-89  -> 75%
         * 90-100 -> 100%
         */
        if (perc < 30)
                image = 0;
        else if ((perc >= 30) && (perc < 70))
                image = 1;
        else if ((perc >= 70) && (perc < 90))
                image = 2;
        else if ((perc >= 90) && (perc <= 100))
                image = 3;
        else
                image = 4;

        scale = gtk_widget_get_scale_factor (GTK_WIDGET (applet->icon));

        if (applet->surfaces[image] == NULL) {
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale (cpufreq_icons[image],
                                                                       size * scale,
                                                                       size * scale,
                                                                       TRUE,
                                                                       NULL);
                applet->surfaces[image] = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale, NULL);
        }

        gtk_image_set_from_surface (GTK_IMAGE (applet->icon), applet->surfaces[image]);
}

static gboolean
refresh_cb (CPUFreqApplet *applet)
{
	cpufreq_applet_refresh (applet);

	return FALSE;
}

static void
cpufreq_applet_update_visibility (CPUFreqApplet *applet)
{
        CPUFreqShowMode     show_mode;
        CPUFreqShowTextMode show_text_mode;
        gboolean            show_freq = FALSE;
        gboolean            show_perc = FALSE;
        gboolean            show_unit = FALSE;
        gboolean            show_icon = FALSE;
        gboolean            changed = FALSE;
        gboolean            need_update = FALSE;

        show_mode = cpufreq_prefs_get_show_mode (applet->prefs);
        show_text_mode = cpufreq_prefs_get_show_text_mode (applet->prefs);

        if (show_mode != CPUFREQ_MODE_GRAPHIC) {
                show_icon = (show_mode == CPUFREQ_MODE_BOTH);

                switch (show_text_mode) {
                case CPUFREQ_MODE_TEXT_FREQUENCY:
                        show_freq = TRUE;
                        break;
                case CPUFREQ_MODE_TEXT_PERCENTAGE:
                        show_perc = TRUE;
                        break;
                case CPUFREQ_MODE_TEXT_FREQUENCY_UNIT:
                        show_freq = TRUE;
                        show_unit = TRUE;
                        break;
                }
        } else {
                show_icon = TRUE;
        }

	if (applet->show_mode != show_mode) {
		applet->show_mode = show_mode;
		need_update = TRUE;
	}

	if (applet->show_text_mode != show_text_mode) {
		applet->show_text_mode = show_text_mode;
		need_update = TRUE;
	}

        if (show_freq != applet->show_freq) {
                applet->show_freq = show_freq;
                changed = TRUE;
        }

        if (show_perc != applet->show_perc) {
                applet->show_perc = show_perc;
                changed = TRUE;
        }

        if (changed) {
                g_object_set (G_OBJECT (applet->label),
                              "visible",
                              applet->show_freq || applet->show_perc,
                              NULL);
        }

        if (show_unit != applet->show_unit) {
                applet->show_unit = show_unit;
                changed = TRUE;

                g_object_set (G_OBJECT (applet->unit_label),
                              "visible", applet->show_unit,
                              NULL);
        }

        if (show_icon != applet->show_icon) {
                applet->show_icon = show_icon;
                changed = TRUE;

                g_object_set (G_OBJECT (applet->icon),
                              "visible", applet->show_icon,
                              NULL);
        }

        if (changed)
		g_idle_add ((GSourceFunc)refresh_cb, applet);

        if (need_update)
                cpufreq_applet_update (applet, applet->monitor);
}

static void
cpufreq_applet_update (CPUFreqApplet *applet, CPUFreqMonitor *monitor)
{
        gchar       *text_mode = NULL;
        gchar       *freq_label, *unit_label;
        gint         freq;
        gint         perc;
        guint        cpu;
        GtkRequisition  req;
        const gchar *governor;

        cpu = cpufreq_monitor_get_cpu (monitor);
        freq = cpufreq_monitor_get_frequency (monitor);
        perc = cpufreq_monitor_get_percentage (monitor);
        governor = cpufreq_monitor_get_governor (monitor);

        freq_label = cpufreq_utils_get_frequency_label (freq);
        unit_label = cpufreq_utils_get_frequency_unit (freq);

        if (applet->show_freq) {
                gtk_label_set_text (GTK_LABEL (applet->label), freq_label);
                /*Hold the largest size set by any jumping text */
                gtk_widget_get_preferred_size (GTK_WIDGET (applet->label),&req, NULL);
                gtk_widget_set_size_request (GTK_WIDGET (applet->label),req.width, req.height);
        }

        if (applet->show_perc) {
                gchar *text_perc;

                text_perc = g_strdup_printf ("%d%%", perc);
                gtk_label_set_text (GTK_LABEL (applet->label), text_perc);
                g_free (text_perc);
        }

        if (applet->show_unit) {
                gtk_label_set_text (GTK_LABEL (applet->unit_label), unit_label);
        }

        if (applet->show_icon) {
                cpufreq_applet_pixmap_set_image (applet, perc);
        }

	if (governor) {
		gchar *gov_text;

		gov_text = g_strdup (governor);
		gov_text[0] = g_ascii_toupper (gov_text[0]);
		text_mode = g_strdup_printf ("%s\n%s %s (%d%%)",
					     gov_text, freq_label,
					     unit_label, perc);
		g_free (gov_text);
	}

        g_free (freq_label);
        g_free (unit_label);

	if (text_mode) {
		gchar *text_tip;

		text_tip = cpufreq_utils_get_n_cpus () == 1 ?
			g_strdup_printf ("%s", text_mode) :
			g_strdup_printf ("CPU %u - %s", cpu, text_mode);
		g_free (text_mode);

		gtk_widget_set_tooltip_text (GTK_WIDGET (applet), text_tip);
		g_free (text_tip);
	}

        /* Call refresh only the first time */
        if (applet->need_refresh) {
                cpufreq_applet_refresh (applet);
		applet->need_refresh = FALSE;
        }
}

static gint
cpufreq_applet_get_widget_size (CPUFreqApplet *applet,
                                GtkWidget     *widget)
{
        GtkRequisition  req;
        gint            size;

        if (!gtk_widget_get_visible (widget))
                return 0;

        gtk_widget_get_preferred_size (widget, &req, NULL);

        switch (applet->orient) {
        case MATE_PANEL_APPLET_ORIENT_LEFT:
        case MATE_PANEL_APPLET_ORIENT_RIGHT:
                size = req.width;
                break;
        case MATE_PANEL_APPLET_ORIENT_UP:
        case MATE_PANEL_APPLET_ORIENT_DOWN:
                size = req.height;
                break;
        default:
                g_assert_not_reached ();
        }

        return size;
}

static void
cpufreq_applet_refresh (CPUFreqApplet *applet)
{
    gint      total_size = 0;
    gint      panel_size, label_size;
    gint      unit_label_size, pixmap_size;
    gint      size_step = 12;
    gboolean  horizontal;
    gboolean  do_unref = FALSE;
    GtkRequisition  req;

    panel_size = applet->size - 1; /* 1 pixel margin */

    horizontal = (applet->orient == MATE_PANEL_APPLET_ORIENT_UP ||
             applet->orient == MATE_PANEL_APPLET_ORIENT_DOWN);

        /* Zero out and reset the size of the label in case the theme is getting smaller */
    gtk_widget_set_size_request (GTK_WIDGET (applet->label), 0, 0);
    gtk_widget_get_preferred_size (GTK_WIDGET (applet->label),&req, NULL);
    gtk_widget_set_size_request (GTK_WIDGET (applet->label),req.width, req.height);
       /* We want a fixed label size, the biggest */
    if (horizontal)
        label_size = cpufreq_applet_get_widget_size (applet, applet->label);
    else
        label_size = cpufreq_applet_get_max_label_width (applet);
        total_size += label_size;

    if (horizontal)
        unit_label_size = cpufreq_applet_get_widget_size (applet, applet->unit_label);
    else
        unit_label_size = cpufreq_applet_get_max_unit_width (applet);
        total_size += unit_label_size;

        pixmap_size = cpufreq_applet_get_widget_size (applet, applet->icon);
        total_size += pixmap_size;

        if (applet->box) {
                do_unref = TRUE;
                g_object_ref (applet->icon);
                gtk_container_remove (GTK_CONTAINER (applet->box), applet->icon);
        if (applet->labels_box) {
                        g_object_ref (applet->label);
                        gtk_container_remove (GTK_CONTAINER (applet->labels_box), applet->label);
                        g_object_ref (applet->unit_label);
                        gtk_container_remove (GTK_CONTAINER (applet->labels_box), applet->unit_label);
                }
                gtk_widget_destroy (applet->box);
        }

    if (horizontal) {
        applet->labels_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        if ((label_size + pixmap_size) <= panel_size)
            applet->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
        else
            applet->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    } else {
                if (total_size <= panel_size) {
                        applet->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
                        applet->labels_box  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
                } else if ((label_size + unit_label_size) <= (panel_size - size_step)) {
                        applet->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
                        applet->labels_box  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
                } else {
                        applet->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
                        applet->labels_box  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
                }
	}

        gtk_box_pack_start (GTK_BOX (applet->labels_box), applet->label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (applet->labels_box), applet->unit_label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (applet->box), applet->icon, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (applet->box), applet->labels_box, FALSE, FALSE, 0);
        gtk_widget_show (applet->labels_box);

        gtk_container_add (GTK_CONTAINER (applet->container), applet->box);
        gtk_widget_show (applet->box);

        if (do_unref) {
                g_object_unref (applet->label);
                g_object_unref (applet->unit_label);
                g_object_unref (applet->icon);
        }
}

/* Preferences callbacks */
static void
cpufreq_applet_prefs_cpu_changed (CPUFreqPrefs  *prefs,
				  GParamSpec    *arg1,
				  CPUFreqApplet *applet)
{
	cpufreq_monitor_set_cpu (applet->monitor,
				 cpufreq_prefs_get_cpu (applet->prefs));
}

static void
cpufreq_applet_prefs_show_mode_changed (CPUFreqPrefs  *prefs,
                                        GParamSpec    *arg1,
                                        CPUFreqApplet *applet)
{
        cpufreq_applet_update_visibility (applet);
}

static void
cpufreq_applet_setup (CPUFreqApplet *applet)
{
	GtkActionGroup *action_group;
	gchar          *ui_path;
        AtkObject      *atk_obj;
        gchar          *prefs_key;
	GSettings      *settings;

	g_set_application_name  (_("CPU Frequency Scaling Monitor"));

	gtk_window_set_default_icon_name ("mate-cpu-frequency-applet");

        /* Preferences */
        if (applet->prefs)
                g_object_unref (applet->prefs);

        settings = mate_panel_applet_settings_new (MATE_PANEL_APPLET (applet), "org.mate.panel.applet.cpufreq");
        applet->prefs = cpufreq_prefs_new (settings);

	g_signal_connect (G_OBJECT (applet->prefs),
			  "notify::cpu",
			  G_CALLBACK (cpufreq_applet_prefs_cpu_changed),
			  (gpointer) applet);
        g_signal_connect (G_OBJECT (applet->prefs),
                          "notify::show-mode",
                          G_CALLBACK (cpufreq_applet_prefs_show_mode_changed),
                          (gpointer) applet);
        g_signal_connect (G_OBJECT (applet->prefs),
                          "notify::show-text-mode",
                          G_CALLBACK (cpufreq_applet_prefs_show_mode_changed),
                          (gpointer) applet);

        /* Monitor */
        applet->monitor = cpufreq_monitor_factory_create_monitor (
                cpufreq_prefs_get_cpu (applet->prefs));
        cpufreq_monitor_run (applet->monitor);
        g_signal_connect_swapped (G_OBJECT (applet->monitor), "changed",
                                  G_CALLBACK (cpufreq_applet_update),
                                  (gpointer) applet);

        /* Setup the menus */
	action_group = gtk_action_group_new ("CPUFreq Applet Actions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group,
				      cpufreq_applet_menu_actions,
				      G_N_ELEMENTS (cpufreq_applet_menu_actions),
				      applet);
	ui_path = g_build_filename (CPUFREQ_MENU_UI_DIR, "cpufreq-applet-menu.xml", NULL);
        mate_panel_applet_setup_menu_from_file (MATE_PANEL_APPLET (applet),
					   ui_path, action_group);
	g_free (ui_path);

        if (mate_panel_applet_get_locked_down (MATE_PANEL_APPLET (applet))) {
		GtkAction *action;

		action = gtk_action_group_get_action (action_group, "CPUFreqPreferences");
		gtk_action_set_visible (action, FALSE);
        }
	g_object_unref (action_group);

        atk_obj = gtk_widget_get_accessible (GTK_WIDGET (applet));

        if (GTK_IS_ACCESSIBLE (atk_obj)) {
                atk_object_set_name (atk_obj, _("CPU Frequency Scaling Monitor"));
                atk_object_set_description (atk_obj, _("This utility shows the current CPU Frequency"));
        }

	cpufreq_applet_update_visibility (applet);

	gtk_widget_show (GTK_WIDGET (applet));
}

static gboolean
cpufreq_applet_factory (CPUFreqApplet *applet, const gchar *iid, gpointer gdata)
{
        gboolean retval = FALSE;

        if (!strcmp (iid, "CPUFreqApplet")) {
                cpufreq_applet_setup (applet);

                retval = TRUE;
        }

        return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("CPUFreqAppletFactory",
				  CPUFREQ_TYPE_APPLET,
				  "cpufreq-applet",
				  (MatePanelAppletFactoryCallback) cpufreq_applet_factory,
				  NULL)
