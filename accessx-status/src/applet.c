/* Keyboard Accessibility Status Applet
 * Copyright 2003, 2004 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <mate-panel-applet.h>
#include <X11/XKBlib.h>

#define XK_MISCELLANY
#define XK_XKB_KEYS

#include <X11/keysymdef.h>
#include "applet.h"

static int xkb_base_event_type = 0;

#define ALT_GRAPH_LED_MASK (0x10)
#define ICON_PADDING 4

typedef enum {
    modifier_Shift = 0,
    modifier_Control,
    modifier_Mod1,
    modifier_Mod2,
    modifier_Mod3,
    modifier_Mod4,
    modifier_Mod5,
    modifier_n
} E_modifiers;

typedef struct {
    unsigned int mask;
    GtkWidget* indicator;
    gchar *icon_name;
} ModifierStruct;

static ModifierStruct modifiers[modifier_n] = {
    [modifier_Shift] = {ShiftMask, NULL, SHIFT_KEY_ICON},
    [modifier_Control] = {ControlMask, NULL, CONTROL_KEY_ICON},
    [modifier_Mod1] = {Mod1Mask, NULL, ALT_KEY_ICON},
    [modifier_Mod2] = {Mod2Mask, NULL, META_KEY_ICON},
    [modifier_Mod3] = {Mod3Mask, NULL, HYPER_KEY_ICON},
    [modifier_Mod4] = {Mod4Mask, NULL, SUPER_KEY_ICON},
    [modifier_Mod5] = {Mod5Mask, NULL, ALTGRAPH_KEY_ICON}
};

typedef struct {
    unsigned int mask;
    gchar* icon_name;
} ButtonIconStruct;

static ButtonIconStruct button_icons[] = {
    {Button1Mask, MOUSEKEYS_BUTTON_LEFT},
    {Button2Mask, MOUSEKEYS_BUTTON_MIDDLE},
    {Button3Mask, MOUSEKEYS_BUTTON_RIGHT}
};

static void
popup_error_dialog (AccessxStatusApplet* sapplet);

/* cribbed from geyes */
static void
about_cb (GtkAction*           action,
          AccessxStatusApplet* sapplet)
{
    static const gchar* authors[] = {
        "Calum Benson <calum.benson@sun.com>",
        "Bill Haneman <bill.haneman@sun.com>",
        NULL
    };

    const gchar* documenters[] = {
        "Bill Haneman <bill.haneman@sun.com>",
        N_("Sun GNOME Documentation Team <gdocteam@sun.com>"),
        N_("MATE Documentation Team"),
        NULL
    };

#ifdef ENABLE_NLS
    const char **p;
    for (p = documenters; *p; ++p)
        *p = _(*p);
#endif

    gtk_show_about_dialog (NULL,
        "title", _("About AccessX Status"),
        "version", VERSION,
        "comments", _("Shows the state of AccessX features such as latched modifiers"),
        "copyright", _("Copyright \xc2\xa9 2003 Sun Microsystems\n"
                       "Copyright \xc2\xa9 2012-2020 MATE developers"),
        "authors", authors,
        "documenters", documenters,
        "translator-credits", _("translator-credits"),
        "logo-icon-name", ACCESSX_APPLET,
        NULL);
}

static void
help_cb (GtkAction*           action,
         AccessxStatusApplet* sapplet)
{
    GError* error = NULL;
    GdkScreen* screen = gtk_widget_get_screen (GTK_WIDGET (sapplet->applet));

    gtk_show_uri_on_window (NULL,
                            "help:mate-accessx-status",
                            gtk_get_current_event_time (),
                            &error);

    if (error)
    {
        GtkWidget* parent = gtk_widget_get_parent (GTK_WIDGET (sapplet->applet));

        GtkWidget* dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                                    _("There was an error launching the help viewer: %s"),
                                                    error->message);

        g_signal_connect (G_OBJECT (dialog),
                          "response",
                          G_CALLBACK (gtk_widget_destroy),
                          NULL);

        gtk_window_set_screen (GTK_WINDOW (dialog), screen);
        gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

        gtk_widget_show (dialog);
        g_error_free (error);
    }
}

static void
dialog_cb (GtkAction*           action,
           AccessxStatusApplet* sapplet)
{
    GError* error = NULL;
    GdkScreen *screen;
    GdkAppLaunchContext *launch_context;
    GAppInfo *appinfo;

    if (sapplet->error_type != ACCESSX_STATUS_ERROR_NONE)
    {
        popup_error_dialog (sapplet);
        return;
    }


    screen = gtk_widget_get_screen (GTK_WIDGET (sapplet->applet));
    appinfo = g_app_info_create_from_commandline ("mate-keyboard-properties --a11y",
                                                  _("Open the keyboard preferences dialog"),
                                                  G_APP_INFO_CREATE_NONE,
                                                  &error);

    if (!error) {
        launch_context = gdk_display_get_app_launch_context (
            gtk_widget_get_display (GTK_WIDGET (sapplet->applet)));
        gdk_app_launch_context_set_screen (launch_context, screen);
        g_app_info_launch (appinfo,
                           NULL,
                           G_APP_LAUNCH_CONTEXT (launch_context),
                           &error);

        g_object_unref (launch_context);
    }

    if (error != NULL)
    {
        GtkWidget* dialog = gtk_message_dialog_new (NULL,
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    _("There was an error launching the keyboard preferences dialog: %s"),
                                                    error->message);

        g_signal_connect (G_OBJECT (dialog),
                          "response",
                          G_CALLBACK (gtk_widget_destroy),
                          NULL);

        gtk_window_set_screen (GTK_WINDOW (dialog), screen);
        gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

        gtk_widget_show (dialog);
        g_error_free (error);
    }

    g_object_unref (appinfo);
}

static const GtkActionEntry accessx_status_applet_menu_actions[] = {
    {"Dialog", "document-properties",
        N_("_Keyboard Accessibility Preferences"),
        NULL, NULL, G_CALLBACK (dialog_cb)},
    {"Help", "help-browser", N_("_Help"),
        NULL, NULL, G_CALLBACK (help_cb)},
    {"About", "help-about", N_("_About"),
        NULL, NULL, G_CALLBACK (about_cb)}
};

static XkbDescPtr
accessx_status_applet_get_xkb_desc (AccessxStatusApplet* sapplet)
{
    Display* display;

    if (sapplet->xkb == NULL)
    {
        int ir, reason_return;
        char* display_name = getenv ("DISPLAY");
        display = XkbOpenDisplay (display_name,
                                  &xkb_base_event_type,
                                  &ir,
                                  NULL, NULL,
                                  &reason_return);
        g_assert (display);

        /* TODO: change error message below to something user-viewable */
        if (display == NULL)
        {
            g_warning ("Xkb extension could not be initialized! (error code %x)",
                       reason_return);
        }
        else
        {
            sapplet->xkb = XkbGetMap (display,
                                      XkbAllComponentsMask,
                                      XkbUseCoreKbd);
        }

        g_assert (sapplet->xkb);

        if (sapplet->xkb == NULL)
        {
            g_warning ("Xkb keyboard description not available!");
        }

        sapplet->xkb_display = display;
    }
    return sapplet->xkb;
}

static gboolean
accessx_status_applet_xkb_select (AccessxStatusApplet* sapplet)
{
    int opcode_rtn, error_rtn;
    gboolean retval = FALSE;
    GdkWindow* window = gtk_widget_get_window (GTK_WIDGET (sapplet->applet));

    g_assert (sapplet && sapplet->applet && window);

    Display* display = GDK_WINDOW_XDISPLAY (window);

    g_assert (display);

    retval = XkbQueryExtension (display,
                                &opcode_rtn,
                                &xkb_base_event_type,
                                &error_rtn,
                                NULL, NULL);

    if (retval)
    {
        retval = XkbSelectEvents (display,
                                  XkbUseCoreKbd,
                                  XkbAllEventsMask,
                                  XkbAllEventsMask);
        sapplet->xkb = accessx_status_applet_get_xkb_desc (sapplet);
    }
    else
    {
        sapplet->error_type = ACCESSX_STATUS_ERROR_XKB_DISABLED;
    }

    return retval;
}

static void
accessx_status_applet_init_modifiers (AccessxStatusApplet* sapplet)
{
    unsigned int hyper_mask, super_mask, alt_gr_mask;

    unsigned int alt_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Alt_L);
    unsigned int meta_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Meta_L);

    g_assert (sapplet->meta_indicator);

    if (meta_mask && (meta_mask != alt_mask))
    {
        gtk_widget_show (sapplet->meta_indicator);
    }
    else
    {
        gtk_widget_hide (sapplet->meta_indicator);
    }

    hyper_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Hyper_L);

    if (hyper_mask)
    {
        gtk_widget_show (sapplet->hyper_indicator);
    }
    else
    {
        gtk_widget_hide (sapplet->hyper_indicator);
    }

    super_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Super_L);

    if (super_mask)
    {
        gtk_widget_show (sapplet->super_indicator);
    }
    else
    {
        gtk_widget_hide (sapplet->super_indicator);
    }

    alt_gr_mask = XkbKeysymToModifiers (sapplet->xkb_display, XK_Mode_switch) |
        XkbKeysymToModifiers (sapplet->xkb_display, XK_ISO_Level3_Shift) |
        XkbKeysymToModifiers (sapplet->xkb_display, XK_ISO_Level3_Latch) |
        XkbKeysymToModifiers (sapplet->xkb_display, XK_ISO_Level3_Lock);

    if (alt_gr_mask)
    {
        gtk_widget_show (sapplet->alt_graph_indicator);
    }
    else
    {
        gtk_widget_hide (sapplet->alt_graph_indicator);
    }

    modifiers[modifier_Shift].indicator = sapplet->shift_indicator;
    modifiers[modifier_Control].indicator = sapplet->ctrl_indicator;
    modifiers[modifier_Mod1].indicator = sapplet->alt_indicator;
    modifiers[modifier_Mod2].indicator = sapplet->meta_indicator;
    modifiers[modifier_Mod3].indicator = sapplet->hyper_indicator;
    modifiers[modifier_Mod4].indicator = sapplet->super_indicator;
    modifiers[modifier_Mod5].indicator = sapplet->alt_graph_indicator;
}

static gboolean
timer_reset_slowkeys_image (AccessxStatusApplet* sapplet)
{
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
    gint icon_size = mate_panel_applet_get_size (sapplet->applet) - ICON_PADDING;
    gint icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));
    cairo_surface_t* surface = gtk_icon_theme_load_surface (icon_theme,
                                                            SLOWKEYS_IDLE_ICON,
                                                            icon_size,
                                                            icon_scale,
                                                            NULL, 0, NULL);

    gtk_image_set_from_surface (GTK_IMAGE (sapplet->slowfoo), surface);
    cairo_surface_destroy (surface);

    return G_SOURCE_REMOVE;
}

static gboolean
timer_reset_bouncekeys_image (AccessxStatusApplet* sapplet)
{
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
    gint icon_size = mate_panel_applet_get_size (sapplet->applet) - ICON_PADDING;
    gint icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));
    cairo_surface_t* surface = gtk_icon_theme_load_surface (icon_theme,
                                                            BOUNCEKEYS_ICON,
                                                            icon_size,
                                                            icon_scale,
                                                            NULL, 0, NULL);

    gtk_image_set_from_surface (GTK_IMAGE (sapplet->bouncefoo), surface);
    cairo_surface_destroy (surface);

    return G_SOURCE_REMOVE;
}

static GdkPixbuf*
accessx_status_applet_get_glyph_pixbuf (GtkWidget* widget,
                                        GdkPixbuf* base,
                                        GdkRGBA*   fg,
                                        gchar*     glyphstring)
{
    GdkPixbuf* glyph_pixbuf;
    cairo_surface_t *surface;
    PangoLayout* layout;
    PangoRectangle ink, logic;
    PangoContext* pango_context;
    PangoFontDescription* font_description;
    static gint font_size = 0;
    gint w = gdk_pixbuf_get_width (base);
    gint h = gdk_pixbuf_get_height (base);
    gint icon_scale = 2;
    cairo_t *cr;

    surface = gdk_window_create_similar_surface (gdk_get_default_root_window (),
                                                 CAIRO_CONTENT_COLOR_ALPHA, w, h);

    pango_context = gtk_widget_get_pango_context (widget);

    font_description = pango_context_get_font_description (pango_context);
    if (font_size == 0)
        font_size = pango_font_description_get_size (font_description);
    pango_font_description_set_size (font_description, font_size * icon_scale);

    layout = pango_layout_new (pango_context);
    pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
    pango_layout_set_text (layout, glyphstring, -1);

    cr = cairo_create (surface);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    gdk_cairo_set_source_rgba (cr, fg);

    pango_layout_get_pixel_extents (layout, &ink, &logic);

    cairo_move_to (cr, (w - ink.x - ink.width)/2, (h - ink.y - ink.height)/2);
    pango_cairo_show_layout (cr, layout);
    cairo_destroy (cr);

    g_object_unref (layout);
    glyph_pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, w, h);
    cairo_surface_destroy (surface);
    return glyph_pixbuf;
}

static cairo_surface_t*
accessx_status_applet_altgraph_image (AccessxStatusApplet *sapplet,
                                      GtkStateFlags       state)
{
    GtkIconTheme *icon_theme;
    GdkPixbuf* pixbuf;
    GdkPixbuf* glyph_pixbuf;
    GdkPixbuf* icon_base;
    cairo_surface_t *surface;
    GdkRGBA fg;
    gchar* icon_name;
    int alpha;
    int icon_size, icon_scale;

    icon_theme = gtk_icon_theme_get_default ();
    icon_size = mate_panel_applet_get_size (sapplet->applet) - ICON_PADDING;
    icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));

    switch (state)
    {
        case GTK_STATE_FLAG_NORMAL:
            icon_name = ACCESSX_BASE_ICON_BASE;
            alpha = 255;
            gdk_rgba_parse (&fg, "black");
            break;
        case GTK_STATE_FLAG_SELECTED:
            icon_name = ACCESSX_BASE_ICON_INVERSE;
            alpha = 255;
            gdk_rgba_parse (&fg, "white");
            break;
        case GTK_STATE_FLAG_INSENSITIVE:
        default:
            icon_name = ACCESSX_BASE_ICON;
            alpha = 63;
            gdk_rgba_parse (&fg, "black");
            break;
    }

    icon_base = gtk_icon_theme_load_icon_for_scale (icon_theme,
                                                    icon_name,
                                                    icon_size,
                                                    icon_scale,
                                                    0, NULL);
    pixbuf = gdk_pixbuf_copy (icon_base);
    g_object_unref (icon_base);
    /*
     * should be N_("ae"));
     * need en_ locale for this.
     */
    /*
     * Translators: substitute an easily-recognized single glyph
     * from Level 2, i.e. an AltGraph character from a common keyboard
     * in your locale.
     */
    glyph_pixbuf = accessx_status_applet_get_glyph_pixbuf (GTK_WIDGET (sapplet->applet),
                                                           pixbuf, &fg, ("æ"));
    gdk_pixbuf_composite (glyph_pixbuf,
                          pixbuf, 0, 0,
                          gdk_pixbuf_get_width (glyph_pixbuf),
                          gdk_pixbuf_get_height (glyph_pixbuf),
                          0., 0., 1.0, 1.0,
                          GDK_INTERP_NEAREST, alpha);
    g_object_unref (glyph_pixbuf);

    surface = gdk_cairo_surface_create_from_pixbuf (pixbuf, icon_scale, NULL);
    g_object_unref (pixbuf);

    return surface;
}

static cairo_surface_t*
accessx_status_applet_slowkeys_image (AccessxStatusApplet*   sapplet,
                                      XkbAccessXNotifyEvent* event)
{
    GdkPixbuf* ret_pixbuf;
    cairo_surface_t *surface;
    GdkWindow* window;
    gboolean is_idle = TRUE;
    gchar* icon_name = SLOWKEYS_IDLE_ICON;
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
    gint icon_size = mate_panel_applet_get_size (sapplet->applet) - ICON_PADDING;
    gint icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));

    if (event != NULL)
    {
        is_idle = FALSE;

        switch (event->detail)
        {
            case XkbAXN_SKPress:
                icon_name = ACCESSX_BASE_ICON;
                break;
            case XkbAXN_SKAccept:
                icon_name = ACCESSX_ACCEPT_BASE;
                break;
            case XkbAXN_SKReject:
                icon_name = ACCESSX_REJECT_BASE;
                g_timeout_add_full (G_PRIORITY_HIGH_IDLE,
                                   MAX (event->sk_delay, 150),
                                   (GSourceFunc)timer_reset_slowkeys_image,
                                   sapplet, NULL);
                break;
            case XkbAXN_SKRelease:
            default:
                icon_name = SLOWKEYS_IDLE_ICON;
                is_idle = TRUE;
                break;
        }
    }

    ret_pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme,
                                                     icon_name,
                                                     icon_size,
                                                     icon_scale,
                                                     0, NULL);

    if (!is_idle)
    {
        GdkPixbuf* glyph_pixbuf;
        GdkPixbuf* tmp_pixbuf;
        GdkRGBA fg;
        gchar* glyphstring = N_("a");
        gint alpha;
        tmp_pixbuf = ret_pixbuf;
        ret_pixbuf = gdk_pixbuf_copy (tmp_pixbuf);
        g_object_unref (tmp_pixbuf);

        window = gtk_widget_get_window (GTK_WIDGET (sapplet->applet));

        if (event && window)
        {
            KeySym keysym = XkbKeycodeToKeysym (GDK_WINDOW_XDISPLAY (window),
                                                event->keycode, 0, 0);
            glyphstring = XKeysymToString (keysym);

            if ((!g_utf8_validate (glyphstring, -1, NULL)) ||
                (g_utf8_strlen (glyphstring, -1) > 1))
            {
                glyphstring = "";
            }
        }

        switch (gtk_widget_get_state_flags (GTK_WIDGET (sapplet->applet)))
        {
            case GTK_STATE_FLAG_NORMAL:
                alpha = 255;
                gdk_rgba_parse (&fg, "black");
                break;
            case GTK_STATE_FLAG_SELECTED:
                alpha = 255;
                gdk_rgba_parse (&fg, "white");
                break;
            case GTK_STATE_FLAG_INSENSITIVE:
            default:
                alpha = 63;
                gdk_rgba_parse (&fg, "black");
                break;
        }

        glyph_pixbuf = accessx_status_applet_get_glyph_pixbuf (GTK_WIDGET (sapplet->applet),
                                                               ret_pixbuf,
                                                               &fg,
                                                               glyphstring);

        gdk_pixbuf_composite (glyph_pixbuf,
                              ret_pixbuf,
                              0, 0,
                              gdk_pixbuf_get_width (glyph_pixbuf),
                              gdk_pixbuf_get_height (glyph_pixbuf),
                              0., 0., 1.0, 1.0,
                              GDK_INTERP_NEAREST, alpha);

        g_object_unref (glyph_pixbuf);
    }

    surface = gdk_cairo_surface_create_from_pixbuf (ret_pixbuf, icon_scale, NULL);
    g_object_unref (ret_pixbuf);

    return surface;
}

static cairo_surface_t*
accessx_status_applet_bouncekeys_image (AccessxStatusApplet*   sapplet,
                                        XkbAccessXNotifyEvent* event)
{
    GdkRGBA fg;
    GdkPixbuf* icon_base = NULL;
    GdkPixbuf* tmp_pixbuf;
    cairo_surface_t *surface;
    /* Note to translators: the first letter of the alphabet, not the indefinite article */
    gchar* glyphstring = N_("a");
    gchar* icon_name = ACCESSX_BASE_ICON;
    gint alpha;
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
    gint icon_size = mate_panel_applet_get_size (sapplet->applet) - ICON_PADDING;
    gint icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));

    g_assert (sapplet->applet);

    switch (gtk_widget_get_state_flags (GTK_WIDGET (sapplet->applet)))
    {
        case GTK_STATE_FLAG_NORMAL:
            alpha = 255;
            gdk_rgba_parse (&fg, "black");
            break;
        case GTK_STATE_FLAG_SELECTED:
            alpha = 255;
            gdk_rgba_parse (&fg, "white");
            break;
        case GTK_STATE_FLAG_INSENSITIVE:
        default:
            alpha = 63;
            gdk_rgba_parse (&fg, "black");
            break;
    }

    if (event != NULL)
    {
        switch (event->detail)
        {
            case XkbAXN_BKAccept:
                icon_name = SLOWKEYS_ACCEPT_ICON;
                break;
            case XkbAXN_BKReject:
                icon_name = SLOWKEYS_REJECT_ICON;
                g_timeout_add_full (G_PRIORITY_HIGH_IDLE,
                                    MAX (event->debounce_delay, 150),
                                    (GSourceFunc)timer_reset_bouncekeys_image,
                                    sapplet, NULL);
                break;
            default:
                icon_name = ACCESSX_BASE_ICON;
                break;
        }
    }
    tmp_pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme,
                                                     icon_name,
                                                     icon_size,
                                                     icon_scale,
                                                     0, NULL);

    if (tmp_pixbuf)
    {
        GdkPixbuf* glyph_pixbuf;
        icon_base = gdk_pixbuf_copy (tmp_pixbuf);
        g_object_unref (tmp_pixbuf);
        glyph_pixbuf = accessx_status_applet_get_glyph_pixbuf (GTK_WIDGET (sapplet->applet),
                                                               icon_base,
                                                               &fg,
                                                               glyphstring);

        gdk_pixbuf_composite (glyph_pixbuf,
                              icon_base,
                              2, 2,
                              gdk_pixbuf_get_width (glyph_pixbuf) - 2,
                              gdk_pixbuf_get_height (glyph_pixbuf) - 2,
                              -2., -2., 1.0, 1.0,
                              GDK_INTERP_NEAREST, 96);

        gdk_pixbuf_composite (glyph_pixbuf,
                              icon_base,
                              1, 1,
                              gdk_pixbuf_get_width (glyph_pixbuf) - 1,
                              gdk_pixbuf_get_height (glyph_pixbuf) - 1,
                              1., 1., 1.0, 1.0,
                              GDK_INTERP_NEAREST, alpha);

        g_object_unref (glyph_pixbuf);
    }

    surface = gdk_cairo_surface_create_from_pixbuf (icon_base,
                                                    icon_scale,
                                                    NULL);
    g_object_unref (icon_base);

    return surface;
}

static cairo_surface_t*
accessx_status_applet_mousekeys_image (AccessxStatusApplet* sapplet,
                                       XkbStateNotifyEvent* event)
{
    GdkPixbuf* mouse_pixbuf = NULL, *button_pixbuf, *dot_pixbuf, *tmp_pixbuf;
    cairo_surface_t *surface;
    gchar* which_dot = MOUSEKEYS_DOT_LEFT;
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
    gint icon_size = mate_panel_applet_get_size (sapplet->applet) - ICON_PADDING;
    gint icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));
    tmp_pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme,
                                                     MOUSEKEYS_BASE_ICON,
                                                     icon_size,
                                                     icon_scale,
                                                     0, NULL);

    mouse_pixbuf = gdk_pixbuf_copy (tmp_pixbuf);
    g_object_unref (tmp_pixbuf);
    /* composite in the buttons */
    if (mouse_pixbuf && event && event->ptr_buttons)
    {
        gint i;

        for (i = 0; i < G_N_ELEMENTS (button_icons); ++i)
        {
            if (event->ptr_buttons & button_icons[i].mask)
            {
                button_pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme,
                                                                    button_icons[i].icon_name,
                                                                    icon_size,
                                                                    icon_scale,
                                                                    0, NULL);

                gdk_pixbuf_composite (button_pixbuf,
                                      mouse_pixbuf,
                                      0, 0,
                                      gdk_pixbuf_get_width (button_pixbuf),
                                      gdk_pixbuf_get_height (button_pixbuf),
                                      0.0, 0.0, 1.0, 1.0,
                                      GDK_INTERP_NEAREST, 255);

                g_object_unref (button_pixbuf);
            }
        }
    }

    if (event)
    {
        switch (sapplet->xkb->ctrls->mk_dflt_btn)
        {
            case Button2:
                which_dot = MOUSEKEYS_DOT_MIDDLE;
                break;
            case Button3:
                which_dot = MOUSEKEYS_DOT_RIGHT;
                break;
            case Button1:
            default:
                which_dot = MOUSEKEYS_DOT_LEFT;
                break;
        }
    }
    dot_pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme,
                                                     which_dot,
                                                     icon_size,
                                                     icon_scale,
                                                     0, NULL);

    gdk_pixbuf_composite (dot_pixbuf,
                          mouse_pixbuf,
                          0, 0,
                          gdk_pixbuf_get_width (dot_pixbuf),
                          gdk_pixbuf_get_height (dot_pixbuf),
                          0.0, 0.0, 1.0, 1.0,
                          GDK_INTERP_NEAREST, 255);

    surface = gdk_cairo_surface_create_from_pixbuf (mouse_pixbuf,
                                                    icon_scale,
                                                    NULL);

    g_object_unref (mouse_pixbuf);
    g_object_unref (dot_pixbuf);

    return surface;
}

static void
accessx_status_applet_set_state_icon (AccessxStatusApplet* sapplet,
                                      ModifierStruct*      modifier,
                                      GtkStateFlags        state)
{
    cairo_surface_t* surface = NULL;
    GtkIconTheme *icon_theme;
    gint icon_size, icon_scale;
    gchar *icon_name = NULL;

    switch (modifier->mask)
    {
        case ShiftMask:
            if (state == GTK_STATE_FLAG_SELECTED)
                icon_name = SHIFT_KEY_ICON_LOCKED;
            else if (state == GTK_STATE_FLAG_NORMAL)
                icon_name = SHIFT_KEY_ICON_LATCHED;
            else
                icon_name = SHIFT_KEY_ICON;
            break;

        case ControlMask:
            if (state == GTK_STATE_FLAG_SELECTED)
                icon_name = CONTROL_KEY_ICON_LOCKED;
            else if (state == GTK_STATE_FLAG_NORMAL)
                icon_name = CONTROL_KEY_ICON_LATCHED;
            else
                icon_name = CONTROL_KEY_ICON;
            break;

        case Mod1Mask:
            if (state == GTK_STATE_FLAG_SELECTED)
                icon_name = ALT_KEY_ICON_LOCKED;
            else if (state == GTK_STATE_FLAG_NORMAL)
                icon_name = ALT_KEY_ICON_LATCHED;
            else
                icon_name = ALT_KEY_ICON;
            break;

        case Mod2Mask:
            if (state == GTK_STATE_FLAG_SELECTED)
                icon_name = META_KEY_ICON_LOCKED;
            else if (state == GTK_STATE_FLAG_NORMAL)
                icon_name = META_KEY_ICON_LATCHED;
            else
                icon_name = META_KEY_ICON;
            break;

        case Mod3Mask:
            if (state == GTK_STATE_FLAG_SELECTED)
                icon_name = HYPER_KEY_ICON_LOCKED;
            else if (state == GTK_STATE_FLAG_NORMAL)
                icon_name = HYPER_KEY_ICON_LATCHED;
            else
                icon_name = HYPER_KEY_ICON;
            break;

        case Mod4Mask:
            if (state == GTK_STATE_FLAG_SELECTED)
                icon_name = SUPER_KEY_ICON_LOCKED;
            else if (state == GTK_STATE_FLAG_NORMAL)
                icon_name = SUPER_KEY_ICON_LATCHED;
            else
                icon_name = SUPER_KEY_ICON;
            break;

        case Mod5Mask:
            surface = accessx_status_applet_altgraph_image (sapplet, state);
            break;
    }

    if (surface == NULL && icon_name != NULL)
    {
        icon_theme = gtk_icon_theme_get_default ();
        icon_size = mate_panel_applet_get_size (sapplet->applet) - ICON_PADDING;
        icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));
        surface = gtk_icon_theme_load_surface (icon_theme,
                                               icon_name,
                                               icon_size,
                                               icon_scale,
                                               NULL, 0, NULL);
    }

    if (surface != NULL)
    {
        gtk_image_set_from_surface (GTK_IMAGE (modifier->indicator), surface);
        cairo_surface_destroy (surface);
    }
}

static void
accessx_status_applet_update (AccessxStatusApplet*    sapplet,
                              AccessxStatusNotifyType notify_type,
                              XkbEvent*               event)
{
    GdkWindow* window;
    gint i;

    window = gtk_widget_get_window (GTK_WIDGET (sapplet->applet));

    if (notify_type & ACCESSX_STATUS_MODIFIERS)
    {
        unsigned int locked_mods = 0, latched_mods = 0;

        if (event != NULL)
        {
            locked_mods = event->state.locked_mods;
            latched_mods = event->state.latched_mods;
        }
        else if (sapplet->applet && window)
        {
            XkbStateRec state;
            XkbGetState (GDK_WINDOW_XDISPLAY (window), XkbUseCoreKbd, &state);
            locked_mods = state.locked_mods;
            latched_mods = state.latched_mods;
        }
        /* determine which modifiers are locked, and set state accordingly */
        for (i = 0; i < modifier_n; ++i)
        {
            if (modifiers[i].indicator != NULL && modifiers[i].mask)
            {
                if (locked_mods & modifiers[i].mask)
                {
                    gtk_widget_set_sensitive (modifiers[i].indicator, TRUE);
                    accessx_status_applet_set_state_icon (sapplet,
                                                          &modifiers[i],
                                                          GTK_STATE_FLAG_SELECTED);
                }
                else if (latched_mods & modifiers[i].mask)
                {
                    gtk_widget_set_sensitive (modifiers[i].indicator, TRUE);
                    accessx_status_applet_set_state_icon (sapplet,
                                                          &modifiers[i],
                                                          GTK_STATE_FLAG_NORMAL);
                }
                else
                {
                    gtk_widget_set_sensitive (modifiers[i].indicator, FALSE);
                    accessx_status_applet_set_state_icon (sapplet,
                                                          &modifiers[i],
                                                          GTK_STATE_FLAG_INSENSITIVE);
                }
            }
        }
    }

    if ((notify_type & ACCESSX_STATUS_SLOWKEYS) && (event != NULL))
    {
        cairo_surface_t* surface = accessx_status_applet_slowkeys_image (sapplet,
                                                                         &event->accessx);
        gtk_image_set_from_surface (GTK_IMAGE (sapplet->slowfoo), surface);
        cairo_surface_destroy (surface);
    }

    if ((notify_type & ACCESSX_STATUS_BOUNCEKEYS) && (event != NULL))
    {
        cairo_surface_t* surface = accessx_status_applet_bouncekeys_image (sapplet,
                                                                           &event->accessx);
        gtk_image_set_from_surface (GTK_IMAGE (sapplet->bouncefoo), surface);
        cairo_surface_destroy (surface);
    }

    if ((notify_type & ACCESSX_STATUS_MOUSEKEYS) && (event != NULL))
    {
        cairo_surface_t* surface = accessx_status_applet_mousekeys_image (sapplet,
                                                                          &event->state);
        gtk_image_set_from_surface (GTK_IMAGE (sapplet->mousefoo), surface);
        cairo_surface_destroy (surface);
    }

    if (notify_type & ACCESSX_STATUS_ENABLED)
    {
        /* Update the visibility of widgets in the box */
        /* XkbMouseKeysMask | XkbStickyKeysMask | XkbSlowKeysMask | XkbBounceKeysMask */
        XkbGetControls (GDK_WINDOW_XDISPLAY (window), XkbAllControlsMask, sapplet->xkb);

        if (!(sapplet->xkb->ctrls->enabled_ctrls &
            (XkbMouseKeysMask | XkbStickyKeysMask | XkbSlowKeysMask | XkbBounceKeysMask)))
        {
            gtk_widget_show (sapplet->idlefoo);
        }
        else
        {
            gtk_widget_hide (sapplet->idlefoo);
        }

        if (sapplet->xkb->ctrls->enabled_ctrls & XkbMouseKeysMask)
        {
            gtk_widget_show (sapplet->mousefoo);
        }
        else
        {
            gtk_widget_hide (sapplet->mousefoo);
        }

        if (sapplet->xkb->ctrls->enabled_ctrls & XkbStickyKeysMask)
        {
            gtk_widget_show (sapplet->stickyfoo);
        }
        else
        {
            gtk_widget_hide (sapplet->stickyfoo);
        }

        if (sapplet->xkb->ctrls->enabled_ctrls & XkbSlowKeysMask)
        {
            gtk_widget_show (sapplet->slowfoo);
        }
        else
        {
            gtk_widget_hide (sapplet->slowfoo);
        }

        if (sapplet->xkb->ctrls->enabled_ctrls & XkbBounceKeysMask)
        {
            gtk_widget_show (sapplet->bouncefoo);
        }
        else
        {
            gtk_widget_hide (sapplet->bouncefoo);
        }
    }

    return;
}

static void
accessx_status_applet_notify_xkb_ax (AccessxStatusApplet*   sapplet,
                                     XkbAccessXNotifyEvent* event)
{
    AccessxStatusNotifyType notify_mask = 0;

    switch (event->detail)
    {
        case XkbAXN_SKPress:
        case XkbAXN_SKAccept:
        case XkbAXN_SKRelease:
        case XkbAXN_SKReject:
            notify_mask |= ACCESSX_STATUS_SLOWKEYS;
            break;
        case XkbAXN_BKAccept:
        case XkbAXN_BKReject:
            notify_mask |= ACCESSX_STATUS_BOUNCEKEYS;
            break;
        case XkbAXN_AXKWarning:
            break;
        default:
            break;
    }

    accessx_status_applet_update (sapplet,
                                  notify_mask,
                                  (XkbEvent*) event);
}

static void
accessx_status_applet_notify_xkb_state (AccessxStatusApplet* sapplet,
                                        XkbStateNotifyEvent* event)
{
    AccessxStatusNotifyType notify_mask = 0;

    if (event->changed & XkbPointerButtonMask)
    {
        notify_mask |= ACCESSX_STATUS_MOUSEKEYS;
    }

    if (event->changed & (XkbModifierLatchMask | XkbModifierLockMask))
    {
        notify_mask |= ACCESSX_STATUS_MODIFIERS;
    }

    accessx_status_applet_update (sapplet,
                                  notify_mask,
                                  (XkbEvent*) event);
}

static void
accessx_status_applet_notify_xkb_device (AccessxStatusApplet*           sapplet,
                                         XkbExtensionDeviceNotifyEvent* event)
{
    if (event->reason == XkbXI_IndicatorStateMask)
    {
        if (event->led_state &= ALT_GRAPH_LED_MASK)
        {
            gtk_widget_set_sensitive (sapplet->alt_graph_indicator, TRUE);
            accessx_status_applet_set_state_icon (sapplet,
                                                  &modifiers[modifier_Mod5],
                                                  GTK_STATE_FLAG_NORMAL);
        }
        else
        {
            gtk_widget_set_sensitive (sapplet->alt_graph_indicator, FALSE);
            accessx_status_applet_set_state_icon (sapplet,
                                                  &modifiers[modifier_Mod5],
                                                  GTK_STATE_FLAG_INSENSITIVE);
        }
    }
}

static void
accessx_status_applet_notify_xkb_controls (AccessxStatusApplet*    sapplet,
                                           XkbControlsNotifyEvent* event)
{
    unsigned int mask = XkbStickyKeysMask | XkbSlowKeysMask | XkbBounceKeysMask | XkbMouseKeysMask;
    unsigned int notify_mask = 0;

    XkbGetControls (sapplet->xkb_display, XkbMouseKeysMask, sapplet->xkb);

    if (event->enabled_ctrl_changes & mask)
    {
        notify_mask = ACCESSX_STATUS_ENABLED;
    }

    if (event->changed_ctrls & XkbMouseKeysMask)
    {
        notify_mask |= ACCESSX_STATUS_MOUSEKEYS;
    }

    if (notify_mask)
    {
        accessx_status_applet_update (sapplet, notify_mask, (XkbEvent*) event);
    }
}

static void
accessx_status_applet_notify_xkb_event (AccessxStatusApplet* sapplet,
                                        XkbEvent*            event)
{
    switch (event->any.xkb_type)
    {
        case XkbStateNotify:
            accessx_status_applet_notify_xkb_state (sapplet, &event->state);
            break;
        case XkbAccessXNotify:
            accessx_status_applet_notify_xkb_ax (sapplet, &event->accessx);
            break;
        case XkbControlsNotify:
            accessx_status_applet_notify_xkb_controls (sapplet, &event->ctrls);
            break;
        case XkbExtensionDeviceNotify:
            /* This is a hack around the fact that XFree86's XKB doesn't give AltGr notifications */
            accessx_status_applet_notify_xkb_device (sapplet, &event->device);
            break;
        default:
            break;
    }
}

static
GdkFilterReturn accessx_status_xkb_filter (GdkXEvent* gdk_xevent,
                                           GdkEvent*  event,
                                           gpointer   user_data)
{
    AccessxStatusApplet* sapplet = user_data;
    XkbEvent* xevent = gdk_xevent;

    if (xevent->any.type == xkb_base_event_type)
    {
        accessx_status_applet_notify_xkb_event (sapplet, xevent);
    }

    return GDK_FILTER_CONTINUE;
}

static void
accessx_status_applet_reparent_widget (GtkWidget*    widget,
                                       GtkContainer* container)
{
    if (widget)
    {
        if (gtk_widget_get_parent (widget))
        {
            g_object_ref (G_OBJECT (widget));
            gtk_container_remove (GTK_CONTAINER (gtk_widget_get_parent (widget)),
                                  widget);
        }

        gtk_container_add (container, widget);
    }
}

static void
accessx_status_applet_layout_box (AccessxStatusApplet* sapplet,
                                  GtkWidget*           box,
                                  GtkWidget*           stickyfoo)
{
    AtkObject* atko;

    accessx_status_applet_reparent_widget (sapplet->shift_indicator,
                                           GTK_CONTAINER (stickyfoo));
    accessx_status_applet_reparent_widget (sapplet->ctrl_indicator,
                                           GTK_CONTAINER (stickyfoo));
    accessx_status_applet_reparent_widget (sapplet->alt_indicator,
                                           GTK_CONTAINER (stickyfoo));
    accessx_status_applet_reparent_widget (sapplet->meta_indicator,
                                           GTK_CONTAINER (stickyfoo));
    accessx_status_applet_reparent_widget (sapplet->hyper_indicator,
                                           GTK_CONTAINER (stickyfoo));
    accessx_status_applet_reparent_widget (sapplet->super_indicator,
                                           GTK_CONTAINER (stickyfoo));
    accessx_status_applet_reparent_widget (sapplet->alt_graph_indicator,
                                           GTK_CONTAINER (stickyfoo));
    accessx_status_applet_reparent_widget (sapplet->idlefoo,
                                           GTK_CONTAINER (box));
    accessx_status_applet_reparent_widget (sapplet->mousefoo,
                                           GTK_CONTAINER (box));
    accessx_status_applet_reparent_widget (stickyfoo,
                                           GTK_CONTAINER (box));
    accessx_status_applet_reparent_widget (sapplet->slowfoo,
                                           GTK_CONTAINER (box));
    accessx_status_applet_reparent_widget (sapplet->bouncefoo,
                                           GTK_CONTAINER (box));

    if (sapplet->stickyfoo)
    {
        gtk_widget_destroy (sapplet->stickyfoo);
    }

    if (sapplet->box)
    {
        gtk_container_remove (GTK_CONTAINER (sapplet->applet), sapplet->box);
    }

    gtk_container_add (GTK_CONTAINER (sapplet->applet), box);
    sapplet->stickyfoo = stickyfoo;
    sapplet->box = box;

    atko = gtk_widget_get_accessible (sapplet->box);
    atk_object_set_name (atko, _("AccessX Status"));
    atk_object_set_description (atko,
                                _("Shows keyboard status when accessibility features are used."));

    gtk_widget_show (sapplet->box);
    gtk_widget_show (GTK_WIDGET (sapplet->applet));

    if (gtk_widget_get_realized (sapplet->box) && sapplet->initialized)
    {
        accessx_status_applet_update (sapplet, ACCESSX_STATUS_ALL, NULL);
    }
}

static void
disable_applet (AccessxStatusApplet* sapplet)
{
    gtk_widget_hide (sapplet->meta_indicator);
    gtk_widget_hide (sapplet->hyper_indicator);
    gtk_widget_hide (sapplet->super_indicator);
    gtk_widget_hide (sapplet->alt_graph_indicator);
    gtk_widget_hide (sapplet->shift_indicator);
    gtk_widget_hide (sapplet->ctrl_indicator);
    gtk_widget_hide (sapplet->alt_indicator);
    gtk_widget_hide (sapplet->mousefoo);
    gtk_widget_hide (sapplet->stickyfoo);
    gtk_widget_hide (sapplet->slowfoo);
    gtk_widget_hide (sapplet->bouncefoo);
}

static void
popup_error_dialog (AccessxStatusApplet* sapplet)
{
    GtkWidget* dialog;
    gchar* error_txt;

    switch (sapplet->error_type)
    {
        case ACCESSX_STATUS_ERROR_XKB_DISABLED:
            error_txt = g_strdup (_("XKB Extension is not enabled"));
            break;

        case ACCESSX_STATUS_ERROR_UNKNOWN:

        default: error_txt = g_strdup (_("Unknown error"));
            break;
    }

    dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Error: %s"),
                                     error_txt);

    g_signal_connect (G_OBJECT (dialog),
                      "response",
                      G_CALLBACK (gtk_widget_destroy),
                      NULL);

    gtk_window_set_screen (GTK_WINDOW (dialog),
                           gtk_widget_get_screen (GTK_WIDGET (sapplet->applet)));

    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    gtk_widget_show (dialog);
    g_free (error_txt);
}

static AccessxStatusApplet*
create_applet (MatePanelApplet* applet)
{
    AccessxStatusApplet* sapplet = g_new0 (AccessxStatusApplet, 1);
    GtkWidget* box;
    GtkWidget* stickyfoo;
    AtkObject* atko;
    cairo_surface_t *surface;
    GtkIconTheme *icon_theme;
    gint icon_size, icon_scale;

    g_set_application_name (_("AccessX Status"));

    sapplet->xkb = NULL;
    sapplet->xkb_display = NULL;
    sapplet->box = NULL;
    sapplet->initialized = False; /* there must be a better way */
    sapplet->error_type = ACCESSX_STATUS_ERROR_NONE;
    sapplet->applet = applet;
    mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);
    sapplet->orient = mate_panel_applet_get_orient (applet);

    if (sapplet->orient == MATE_PANEL_APPLET_ORIENT_LEFT ||
        sapplet->orient == MATE_PANEL_APPLET_ORIENT_RIGHT)
    {
        box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        stickyfoo = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    }
    else
    {
        box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        stickyfoo = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    }

    gtk_box_set_homogeneous (GTK_BOX (stickyfoo), TRUE);

    icon_theme = gtk_icon_theme_get_default ();
    icon_size = mate_panel_applet_get_size (sapplet->applet) - ICON_PADDING;
    icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));

    surface = accessx_status_applet_mousekeys_image (sapplet, NULL);
    sapplet->mousefoo = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);
    gtk_widget_hide (sapplet->mousefoo);

    surface = gtk_icon_theme_load_surface (icon_theme,
                                           SHIFT_KEY_ICON,
                                           icon_size,
                                           icon_scale,
                                           NULL, 0, NULL);

    sapplet->shift_indicator = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);

    surface = gtk_icon_theme_load_surface (icon_theme,
                                           CONTROL_KEY_ICON,
                                           icon_size, icon_scale,
                                           NULL, 0, NULL);

    sapplet->ctrl_indicator = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);

    surface = gtk_icon_theme_load_surface (icon_theme,
                                           ALT_KEY_ICON,
                                           icon_size,
                                           icon_scale,
                                           NULL, 0, NULL);

    sapplet->alt_indicator = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);

    surface = gtk_icon_theme_load_surface (icon_theme,
                                           META_KEY_ICON,
                                           icon_size,
                                           icon_scale,
                                           NULL, 0, NULL);

    sapplet->meta_indicator = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);
    gtk_widget_set_sensitive (sapplet->meta_indicator, FALSE);
    gtk_widget_hide (sapplet->meta_indicator);

    surface = gtk_icon_theme_load_surface (icon_theme,
                                           HYPER_KEY_ICON,
                                           icon_size,
                                           icon_scale,
                                           NULL, 0, NULL);

    sapplet->hyper_indicator = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);
    gtk_widget_set_sensitive (sapplet->hyper_indicator, FALSE);
    gtk_widget_hide (sapplet->hyper_indicator);

    surface = gtk_icon_theme_load_surface (icon_theme,
                                           SUPER_KEY_ICON,
                                           icon_size,
                                           icon_scale,
                                           NULL, 0, NULL);

    sapplet->super_indicator = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);
    gtk_widget_set_sensitive (sapplet->super_indicator, FALSE);
    gtk_widget_hide (sapplet->super_indicator);

    surface = accessx_status_applet_altgraph_image (sapplet,
                                                    GTK_STATE_FLAG_NORMAL);

    sapplet->alt_graph_indicator = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);
    gtk_widget_set_sensitive (sapplet->alt_graph_indicator, FALSE);

    surface = accessx_status_applet_slowkeys_image (sapplet, NULL);
    sapplet->slowfoo = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);
    gtk_widget_hide (sapplet->slowfoo);

    surface = accessx_status_applet_bouncekeys_image (sapplet, NULL);
    sapplet->bouncefoo = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);
    gtk_widget_hide (sapplet->bouncefoo);

    surface = gtk_icon_theme_load_surface (icon_theme,
                                           ACCESSX_APPLET,
                                           icon_size,
                                           icon_scale,
                                           NULL, 0, NULL);

    sapplet->idlefoo = gtk_image_new_from_surface (surface);
    cairo_surface_destroy (surface);
    gtk_widget_show (sapplet->idlefoo);

    accessx_status_applet_layout_box (sapplet, box, stickyfoo);
    atko = gtk_widget_get_accessible (GTK_WIDGET (sapplet->applet));
    atk_object_set_name (atko, _("AccessX Status"));
    atk_object_set_description (atko,
                                _("Shows keyboard status when accessibility features are used."));
    return sapplet;
}

static void
accessx_status_applet_destroy (GtkWidget* widget,
                               gpointer   user_data)
{
    AccessxStatusApplet* sapplet = user_data;
    /* do we need to free the icon factory ? */

    gdk_window_remove_filter (NULL, accessx_status_xkb_filter, sapplet);

    if (sapplet->xkb)
    {
        XkbFreeKeyboard (sapplet->xkb, 0, True);
    }

    if (sapplet->xkb_display)
    {
        XCloseDisplay (sapplet->xkb_display);
    }
}

static void
accessx_status_applet_reorient (GtkWidget*            widget,
                                MatePanelAppletOrient o,
                                gpointer              user_data)
{
    AccessxStatusApplet* sapplet = user_data;
    GtkWidget* box;
    GtkWidget* stickyfoo;

    sapplet->orient = o;

    if (o == MATE_PANEL_APPLET_ORIENT_LEFT || o == MATE_PANEL_APPLET_ORIENT_RIGHT)
    {
        box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        stickyfoo = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    }
    else
    {
        box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
        stickyfoo = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    }
    gtk_box_set_homogeneous (GTK_BOX (stickyfoo), TRUE);
    accessx_status_applet_layout_box (sapplet, box, stickyfoo);
}

static void
accessx_status_applet_resize (GtkWidget* widget,
                              int        size,
                              gpointer   user_data)
{
    cairo_surface_t *surface;

    AccessxStatusApplet* sapplet = user_data;
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
    gint icon_scale = gtk_widget_get_scale_factor (GTK_WIDGET (sapplet->applet));

    accessx_status_applet_update (sapplet, ACCESSX_STATUS_ALL, NULL);

    surface = accessx_status_applet_slowkeys_image (sapplet, NULL);
    gtk_image_set_from_surface (GTK_IMAGE (sapplet->slowfoo), surface);
    cairo_surface_destroy (surface);

    surface = accessx_status_applet_bouncekeys_image (sapplet, NULL);
    gtk_image_set_from_surface (GTK_IMAGE (sapplet->bouncefoo), surface);
    cairo_surface_destroy (surface);

    surface = accessx_status_applet_mousekeys_image (sapplet, NULL);
    gtk_image_set_from_surface (GTK_IMAGE (sapplet->mousefoo), surface);
    cairo_surface_destroy (surface);

    surface = gtk_icon_theme_load_surface (icon_theme,
                                           ACCESSX_APPLET, size - ICON_PADDING,
                                           icon_scale,
                                           NULL, 0, NULL);

    gtk_image_set_from_surface (GTK_IMAGE (sapplet->idlefoo), surface);
    cairo_surface_destroy (surface);
}

static gboolean
button_press_cb (GtkWidget*           widget,
                 GdkEventButton*      event,
                 AccessxStatusApplet* sapplet)
{
    if (event->button == 1 && event->type == GDK_BUTTON_PRESS)
    {
        dialog_cb (NULL, sapplet);
    }

    return FALSE;
}

static gboolean
key_press_cb (GtkWidget*           widget,
              GdkEventKey*         event,
              AccessxStatusApplet* sapplet)
{
    switch (event->keyval)
    {
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
        case GDK_KEY_3270_Enter:
        case GDK_KEY_Return:
        case GDK_KEY_space:
        case GDK_KEY_KP_Space:
            dialog_cb (NULL, sapplet);
            return TRUE;

        default:
            break;
    }

    return FALSE;
}

static gboolean
accessx_status_applet_reset (gpointer user_data)
{
    AccessxStatusApplet* sapplet = user_data;
    g_assert (sapplet->applet);
    accessx_status_applet_reorient (GTK_WIDGET (sapplet->applet),
                                    mate_panel_applet_get_orient (sapplet->applet),
                                    sapplet);

    return FALSE;
}

static gboolean
accessx_status_applet_initialize (AccessxStatusApplet* sapplet)
{
    if (!sapplet->initialized)
    {
        sapplet->initialized = True;

        if (!accessx_status_applet_xkb_select (sapplet))
        {
            disable_applet (sapplet);
            popup_error_dialog (sapplet);
            return FALSE ;
        }

        gdk_window_add_filter (NULL, accessx_status_xkb_filter, sapplet);
    }

    accessx_status_applet_init_modifiers (sapplet);
    accessx_status_applet_update (sapplet, ACCESSX_STATUS_ALL, NULL);

    return TRUE;
}

static void
accessx_status_applet_realize (GtkWidget* widget,
                               gpointer   user_data)
{
    AccessxStatusApplet* sapplet = user_data;

    if (!accessx_status_applet_initialize (sapplet))
    {
        return;
    }

    g_idle_add (accessx_status_applet_reset, sapplet);

    return;
}

static gboolean
accessx_status_applet_fill (MatePanelApplet* applet)
{
    AccessxStatusApplet* sapplet;
    AtkObject* atk_object;
    GtkActionGroup* action_group;
    gboolean was_realized = FALSE;

    sapplet = create_applet (applet);

    if (!gtk_widget_get_realized (sapplet->box))
    {
        g_signal_connect_after (G_OBJECT (sapplet->box),
                                "realize",
                                G_CALLBACK (accessx_status_applet_realize),
                                sapplet);
    }
    else
    {
        accessx_status_applet_initialize (sapplet);
        was_realized = TRUE;
    }

    g_object_connect (sapplet->applet,
        "signal::destroy", accessx_status_applet_destroy, sapplet,
        "signal::change_orient", accessx_status_applet_reorient, sapplet,
        "signal::change_size", accessx_status_applet_resize, sapplet,
        NULL);

    g_signal_connect (sapplet->applet,
                      "button_press_event",
                      G_CALLBACK (button_press_cb),
                      sapplet);

    g_signal_connect (sapplet->applet,
                      "key_press_event",
                      G_CALLBACK (key_press_cb),
                      sapplet);

    action_group = gtk_action_group_new ("Accessx Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group,
                                  accessx_status_applet_menu_actions,
                                  G_N_ELEMENTS (accessx_status_applet_menu_actions),
                                  sapplet);

    mate_panel_applet_setup_menu_from_resource (sapplet->applet,
                                                ACCESSX_RESOURCE_PATH "accessx-status-applet-menu.xml",
                                                action_group);

    if (mate_panel_applet_get_locked_down (sapplet->applet))
    {
        GtkAction* action = gtk_action_group_get_action (action_group, "Dialog");
        gtk_action_set_visible (action, FALSE);
    }

    g_object_unref (action_group);

    gtk_widget_set_tooltip_text (GTK_WIDGET (sapplet->applet),
                                 _("Keyboard Accessibility Status"));

    atk_object = gtk_widget_get_accessible (GTK_WIDGET (sapplet->applet));
    atk_object_set_name (atk_object, _("AccessX Status"));
    atk_object_set_description (atk_object,
                                _("Displays current state of keyboard accessibility features"));
    gtk_widget_show_all (GTK_WIDGET (sapplet->applet));

    if (was_realized)
    {
        accessx_status_applet_reset (sapplet);
    }

    return TRUE;
}

static gboolean
accessx_status_applet_factory (MatePanelApplet* applet,
                               const gchar*     iid,
                               gpointer         data)
{
    gboolean retval = FALSE;

    if (!strcmp (iid, "AccessxStatusApplet"))
    {
        retval = accessx_status_applet_fill (applet);
    }

    return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("AccessxStatusAppletFactory",
                                       PANEL_TYPE_APPLET,
                                       "accessx-status",
                                       accessx_status_applet_factory,
                                       NULL)

