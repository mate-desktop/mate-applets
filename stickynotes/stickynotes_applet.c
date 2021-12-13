/* Sticky Notes
 * Copyright (C) 2002-2003 Loban A Rahman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include "stickynotes_applet.h"
#include "stickynotes_applet_callbacks.h"
#include "stickynotes.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

StickyNotes *stickynotes = NULL;

/* Popup menu on the applet */
static const GtkActionEntry stickynotes_applet_menu_actions[] =
{
    { "new_note", "document-new", N_("_New Note"),
      NULL, NULL,
      G_CALLBACK (menu_new_note_cb) },
    { "hide_notes", NULL, N_("Hi_de Notes"),
      NULL, NULL,
      G_CALLBACK (menu_hide_notes_cb) },
    { "destroy_all", "edit-delete", N_("_Delete Notes"),
      NULL, NULL,
      G_CALLBACK (menu_destroy_all_cb) },
    { "preferences", "document-properties", N_("_Preferences"),
      NULL, NULL,
      G_CALLBACK (menu_preferences_cb) },
    { "help", "help-browser", N_("_Help"),
      NULL, NULL,
      G_CALLBACK (menu_help_cb) },
    { "about", "help-about", N_("_About"),
      NULL, NULL,
      G_CALLBACK (menu_about_cb) }
};

static const GtkToggleActionEntry stickynotes_applet_menu_toggle_actions[] =
{
    { "lock", NULL, N_("_Lock Notes"),
      NULL, NULL,
      G_CALLBACK (menu_toggle_lock_cb), FALSE }
};

/* Sticky Notes applet factory */
static gboolean
stickynotes_applet_factory (MatePanelApplet *mate_panel_applet,
                            const gchar     *iid,
                            gpointer         data)
{
    if (!strcmp (iid, "StickyNotesApplet")) {
        if (!stickynotes)
            stickynotes_applet_init (mate_panel_applet);

        mate_panel_applet_set_flags (mate_panel_applet, MATE_PANEL_APPLET_EXPAND_MINOR);

        /* Add applet to linked list of all applets */
        stickynotes->applets = g_list_append (stickynotes->applets,
                                              stickynotes_applet_new (mate_panel_applet));

        stickynotes_applet_update_menus ();
        stickynotes_applet_update_tooltips ();

        return TRUE;
    }

    return FALSE;
}

/* Sticky Notes applet factory */
MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("StickyNotesAppletFactory",
                                       PANEL_TYPE_APPLET,
                                       "stickynotes_applet",
                                       stickynotes_applet_factory,
                                       NULL)

/* colorshift a surface */
static void
stickynotes_make_prelight_icon (cairo_surface_t *dest,
                                cairo_surface_t *src,
                                int              shift)
{
    gint i, j;
    gint width, height, has_alpha, srcrowstride, destrowstride;
    guchar *target_pixels;
    guchar *original_pixels;
    guchar *pixsrc;
    guchar *pixdest;
    int val;
    guchar r,g,b;

    has_alpha = cairo_surface_get_content (src) != CAIRO_CONTENT_COLOR;
    width = cairo_image_surface_get_width (src);
    height = cairo_image_surface_get_height (src);
    srcrowstride = cairo_image_surface_get_stride (src);
    destrowstride = cairo_image_surface_get_stride (dest);
    original_pixels = cairo_image_surface_get_data (src);
    target_pixels = cairo_image_surface_get_data (dest);

    for (i = 0; i < height; i++) {
        pixdest = target_pixels + i*destrowstride;
        pixsrc = original_pixels + i*srcrowstride;
        for (j = 0; j < width; j++) {
            r = *(pixsrc++);
            g = *(pixsrc++);
            b = *(pixsrc++);
            val = r + shift;
            *(pixdest++) = CLAMP (val, 0, 255);
            val = g + shift;
            *(pixdest++) = CLAMP (val, 0, 255);
            val = b + shift;
            *(pixdest++) = CLAMP (val, 0, 255);
            if (has_alpha)
                *(pixdest++) = *(pixsrc++);
        }
    }
}

static void
icon_theme_changed (GtkIconTheme *icon_theme,
                    gpointer      user_data)
{
    gtk_icon_theme_append_search_path (icon_theme,
                                       PKG_DATA_DIR G_DIR_SEPARATOR_S "icons");
}

static void
stickynotes_applet_init_icons (void)
{
    GtkIconTheme *icon_theme;

    icon_theme = gtk_icon_theme_get_default ();
    icon_theme_changed (icon_theme, NULL);

    g_signal_connect (gtk_icon_theme_get_default (), "changed",
                      G_CALLBACK (icon_theme_changed), NULL);
}

static void
stickynotes_destroy (GtkWidget *widget,
                     gpointer   user_dta)
{
    g_signal_handlers_disconnect_by_func (gtk_icon_theme_get_default (),
                                          icon_theme_changed, NULL);
}

/* Create and initalize global sticky notes instance */
void
stickynotes_applet_init (MatePanelApplet *mate_panel_applet)
{
    cairo_t *cr;
    gint size, scale;
    int screen_height;

    stickynotes = g_new (StickyNotes, 1);

    stickynotes->notes = NULL;
    stickynotes->applets = NULL;
    stickynotes->last_timeout_data = 0;

    size = mate_panel_applet_get_size (mate_panel_applet);
    scale = gtk_widget_get_scale_factor (GTK_WIDGET (mate_panel_applet));

    g_set_application_name (_("Sticky Notes"));
    gtk_window_set_default_icon_name ("mate-sticky-notes-applet");

    stickynotes->icon_normal =
        gtk_icon_theme_load_surface (gtk_icon_theme_get_default (),
                                     "mate-sticky-notes-applet",
                                     size, scale, NULL, 0, NULL);

    stickynotes->icon_prelight =
        cairo_surface_create_similar (stickynotes->icon_normal,
                                      cairo_surface_get_content (stickynotes->icon_normal),
                                      cairo_image_surface_get_width (stickynotes->icon_normal),
                                      cairo_image_surface_get_height (stickynotes->icon_normal));

    stickynotes_make_prelight_icon (stickynotes->icon_prelight,
                                    stickynotes->icon_normal, 30);

    cr = cairo_create (stickynotes->icon_prelight);
    cairo_set_operator (cr, CAIRO_OPERATOR_DEST_IN);
    cairo_mask_surface (cr, stickynotes->icon_normal, 0, 0);

    stickynotes->settings = g_settings_new (STICKYNOTES_SCHEMA);
    stickynotes->visible = TRUE;

    stickynotes_applet_init_icons ();
    stickynotes_applet_init_prefs ();

    /* Watch GSettings values */
    g_signal_connect (stickynotes->settings, "changed",
                      G_CALLBACK (preferences_apply_cb), NULL);

    /* Max height for large notes*/
    screen_height = HeightOfScreen (gdk_x11_screen_get_xscreen (gdk_screen_get_default ()));
    stickynotes->max_height = (int) (0.8 * (double) screen_height);

    /* Load sticky notes */
    stickynotes_load (gtk_widget_get_screen (GTK_WIDGET (mate_panel_applet)));

    install_check_click_on_desktop ();
}

void
stickynotes_applet_init_prefs (void)
{

    stickynotes->builder = gtk_builder_new ();

    gtk_builder_add_from_resource (stickynotes->builder,
                                   GRESOURCE "sticky-notes-preferences.ui",
                                   NULL);

    stickynotes->w_prefs =
        GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                            "preferences_dialog"));

    stickynotes->w_prefs_width =
        gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (gtk_builder_get_object (stickynotes->builder,
                                                                                 "width_spin")));
    stickynotes->w_prefs_height =
        gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (gtk_builder_get_object (stickynotes->builder,
                                                                                 "height_spin")));
    stickynotes->w_prefs_color =
        GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                            "default_color"));
    stickynotes->w_prefs_font_color =
        GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                            "prefs_font_color"));
    stickynotes->w_prefs_sys_color =
        GTK_WIDGET (&GTK_CHECK_BUTTON (gtk_builder_get_object (stickynotes->builder,
                                                               "sys_color_check"))->toggle_button);
    stickynotes->w_prefs_font =
        GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                            "default_font"));
    stickynotes->w_prefs_sys_font =
        GTK_WIDGET (&GTK_CHECK_BUTTON (gtk_builder_get_object (stickynotes->builder,
                                                               "sys_font_check"))->toggle_button);
    stickynotes->w_prefs_sticky =
        GTK_WIDGET (&GTK_CHECK_BUTTON (gtk_builder_get_object (stickynotes->builder,
                                                               "sticky_check"))->toggle_button);
    stickynotes->w_prefs_force =
        GTK_WIDGET (&GTK_CHECK_BUTTON (gtk_builder_get_object (stickynotes->builder,
                                                               "force_default_check"))->toggle_button);
    stickynotes->w_prefs_desktop =
        GTK_WIDGET (&GTK_CHECK_BUTTON (gtk_builder_get_object (stickynotes->builder,
                                                               "desktop_hide_check"))->toggle_button);

    g_signal_connect (stickynotes->w_prefs, "response",
                      G_CALLBACK (preferences_response_cb), NULL);
    g_signal_connect (stickynotes->w_prefs, "delete-event",
                      G_CALLBACK (preferences_delete_cb), NULL);
    g_signal_connect_swapped (stickynotes->w_prefs_width, "value-changed",
                              G_CALLBACK (preferences_save_cb), NULL);
    g_signal_connect_swapped (stickynotes->w_prefs_height, "value-changed",
                              G_CALLBACK (preferences_save_cb), NULL);
    g_signal_connect_swapped (stickynotes->w_prefs_sys_color, "toggled",
                              G_CALLBACK (preferences_save_cb), NULL);
    g_signal_connect_swapped (stickynotes->w_prefs_sys_font, "toggled",
                              G_CALLBACK (preferences_save_cb), NULL);
    g_signal_connect (stickynotes->w_prefs_color, "color-set",
                      G_CALLBACK (preferences_color_cb), NULL);
    g_signal_connect (stickynotes->w_prefs_font_color, "color-set",
                      G_CALLBACK (preferences_color_cb), NULL);
    g_signal_connect (stickynotes->w_prefs_font, "font-set",
                      G_CALLBACK (preferences_font_cb), NULL);
    g_signal_connect_swapped (stickynotes->w_prefs_sticky, "toggled",
                              G_CALLBACK (preferences_save_cb), NULL);
    g_signal_connect_swapped (stickynotes->w_prefs_force, "toggled",
                              G_CALLBACK (preferences_save_cb), NULL);
    g_signal_connect_swapped (stickynotes->w_prefs_desktop, "toggled",
                              G_CALLBACK (preferences_save_cb), NULL);

    {
        GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

        gtk_size_group_add_widget (group,
                                   GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                       "width_label")));
        gtk_size_group_add_widget (group,
                                   GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                       "height_label")));
        gtk_size_group_add_widget (group,
                                   GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                       "prefs_color_label")));

        g_object_unref (group);
    }

    if (!g_settings_is_writable (stickynotes->settings, "default-width")) {
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "width_label")),
                                                                      FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "width_spin")),
                                                                      FALSE);
    }
    if (!g_settings_is_writable (stickynotes->settings, "default-height")) {
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "height_label")),
                                                                       FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "height_spin")),
                                                                      FALSE);
    }
    if (!g_settings_is_writable (stickynotes->settings, "default-color")) {
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "prefs_color_label")),
                                                                      FALSE);
        gtk_widget_set_sensitive (stickynotes->w_prefs_color, FALSE);
    }
    if (!g_settings_is_writable (stickynotes->settings, "default-font-color")) {
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "prefs_font_color_label")),
                                                                      FALSE);
        gtk_widget_set_sensitive (stickynotes->w_prefs_font_color, FALSE);
    }
    if (!g_settings_is_writable (stickynotes->settings, "use-system-color"))
        gtk_widget_set_sensitive (stickynotes->w_prefs_sys_color, FALSE);
    if (!g_settings_is_writable (stickynotes->settings, "default-font")) {
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "prefs_font_label")),
                                                                      FALSE);
        gtk_widget_set_sensitive (stickynotes->w_prefs_font, FALSE);
    }
    if (!g_settings_is_writable (stickynotes->settings, "use-system-font"))
        gtk_widget_set_sensitive (stickynotes->w_prefs_sys_font, FALSE);
    if (!g_settings_is_writable (stickynotes->settings, "sticky"))
        gtk_widget_set_sensitive (stickynotes->w_prefs_sticky, FALSE);
    if (!g_settings_is_writable (stickynotes->settings, "force-default"))
        gtk_widget_set_sensitive (stickynotes->w_prefs_force, FALSE);

    stickynotes_applet_update_prefs ();
}

/* Create a Sticky Notes applet */
StickyNotesApplet *
stickynotes_applet_new (MatePanelApplet *mate_panel_applet)
{
    AtkObject *atk_obj;

    /* Create Sticky Notes Applet */
    StickyNotesApplet *applet = g_new (StickyNotesApplet, 1);

    /* Initialize Sticky Notes Applet */
    applet->w_applet = GTK_WIDGET (mate_panel_applet);
    applet->w_image = gtk_image_new ();
    applet->destroy_all_dialog = NULL;
    applet->prelighted = FALSE;

    applet->menu_tip = NULL;

    /* Expand the applet for Fitts' law complience. */
    mate_panel_applet_set_flags (mate_panel_applet,
                                 MATE_PANEL_APPLET_EXPAND_MINOR);

    /* Add the applet icon */
    gtk_container_add (GTK_CONTAINER (mate_panel_applet),
                                      applet->w_image);
    applet->panel_size = mate_panel_applet_get_size (mate_panel_applet);
    applet->panel_orient = mate_panel_applet_get_orient (mate_panel_applet);
    stickynotes_applet_update_icon (applet);

    /* Add the popup menu */
    applet->action_group = gtk_action_group_new ("StickyNotes Applet Actions");
    gtk_action_group_set_translation_domain (applet->action_group,
                                             GETTEXT_PACKAGE);
    gtk_action_group_add_actions (applet->action_group,
                                  stickynotes_applet_menu_actions,
                                  G_N_ELEMENTS (stickynotes_applet_menu_actions),
                                  applet);
    gtk_action_group_add_toggle_actions (applet->action_group,
                                         stickynotes_applet_menu_toggle_actions,
                                         G_N_ELEMENTS (stickynotes_applet_menu_toggle_actions),
                                         applet);
    mate_panel_applet_setup_menu_from_resource (mate_panel_applet,
                                                GRESOURCE "menu.xml",
                                                applet->action_group);

    if (mate_panel_applet_get_locked_down (mate_panel_applet)) {
        GtkAction *action;

        action = gtk_action_group_get_action (applet->action_group,
                                              "preferences");
        gtk_action_set_visible (action, FALSE);
    }

    /* Connect all signals for applet management */
    g_signal_connect (applet->w_applet, "button-press-event",
                      G_CALLBACK (applet_button_cb), applet);
    g_signal_connect (applet->w_applet, "key-press-event",
                      G_CALLBACK (applet_key_cb), applet);
    g_signal_connect (applet->w_applet, "focus-in-event",
                      G_CALLBACK (applet_focus_cb), applet);
    g_signal_connect (applet->w_applet, "focus-out-event",
                      G_CALLBACK (applet_focus_cb), applet);
    g_signal_connect (applet->w_applet, "enter-notify-event",
                      G_CALLBACK (applet_cross_cb), applet);
    g_signal_connect (applet->w_applet, "leave-notify-event",
                      G_CALLBACK (applet_cross_cb), applet);
    g_signal_connect (applet->w_applet, "size-allocate",
                      G_CALLBACK (applet_size_allocate_cb), applet);
    g_signal_connect (applet->w_applet, "change-orient",
                      G_CALLBACK (applet_change_orient_cb), applet);
    g_signal_connect (applet->w_applet, "destroy",
                      G_CALLBACK (applet_destroy_cb), applet);
    g_signal_connect (applet->w_applet, "destroy",
                      G_CALLBACK (stickynotes_destroy), NULL);

    atk_obj = gtk_widget_get_accessible (applet->w_applet);
    atk_object_set_name (atk_obj, _("Sticky Notes"));

    /* Show the applet */
    gtk_widget_show_all (applet->w_applet);

    return applet;
}

void
stickynotes_applet_update_icon (StickyNotesApplet *applet)
{
    cairo_t *cr;
    cairo_surface_t *surface;

    gint size = applet->panel_size;

    if (size > 3)
        size = size - 3;

    /* Choose appropriate icon and size it */
    if (applet->prelighted)
        surface = cairo_surface_create_similar (stickynotes->icon_prelight,
                                                cairo_surface_get_content (stickynotes->icon_prelight),
                                                size, size);
    else
        surface = cairo_surface_create_similar (stickynotes->icon_normal,
                                                cairo_surface_get_content (stickynotes->icon_normal),
                                                size, size);

    cr = cairo_create (surface);
    cairo_set_source_surface (cr, applet->prelighted ? stickynotes->icon_prelight : stickynotes->icon_normal, 0, 0);
    cairo_paint (cr);

    /* Apply the finished surface to the applet image */
    gtk_image_set_from_surface (GTK_IMAGE (applet->w_image), surface);

    cairo_surface_destroy (surface);
}

void
stickynotes_applet_update_prefs (void)
{
    gint width, height;
    gboolean sys_color, sys_font, sticky, force_default, desktop_hide;
    char *font_str;
    char *color_str, *font_color_str;
    GdkRGBA color, font_color;

    width = g_settings_get_int (stickynotes->settings,
                                "default-width");
    width = MAX (width, 1);
    height = g_settings_get_int (stickynotes->settings,
                                 "default-height");
    height = MAX (height, 1);

    sys_color = g_settings_get_boolean (stickynotes->settings,
                                        "use-system-color");
    sys_font = g_settings_get_boolean (stickynotes->settings,
                                       "use-system-font");
    sticky = g_settings_get_boolean (stickynotes->settings,
                                     "sticky");
    force_default = g_settings_get_boolean (stickynotes->settings,
                                            "force-default");
    desktop_hide = g_settings_get_boolean (stickynotes->settings,
                                           "desktop-hide");

    font_str = g_settings_get_string (stickynotes->settings,
                                      "default-font");
    if (!font_str)
        font_str = g_strdup ("Sans 10");

    color_str = g_settings_get_string (stickynotes->settings,
                                       "default-color");
    if (!color_str)
        color_str = g_strdup ("#ECF833");

    font_color_str = g_settings_get_string (stickynotes->settings,
                                            "default-font-color");
    if (!font_color_str)
        font_color_str = g_strdup ("#000000");

    gdk_rgba_parse (&color, color_str);
    gdk_rgba_parse (&font_color, font_color_str);

    g_free (color_str);
    g_free (font_color_str);

    gtk_adjustment_set_value (stickynotes->w_prefs_width, width);
    gtk_adjustment_set_value (stickynotes->w_prefs_height, height);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stickynotes->w_prefs_sys_color),
                                  sys_color);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stickynotes->w_prefs_sys_font),
                                  sys_font);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stickynotes->w_prefs_sticky),
                                  sticky);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stickynotes->w_prefs_force),
                                  force_default);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (stickynotes->w_prefs_desktop),
                                  desktop_hide);

    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (stickynotes->w_prefs_color),
                                &color);
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (stickynotes->w_prefs_font_color),
                                &font_color);

    gtk_font_button_set_font_name (GTK_FONT_BUTTON (stickynotes->w_prefs_font),
                                   font_str);
    g_free (font_str);

    if (g_settings_is_writable (stickynotes->settings, "default-color")) {
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "prefs_color_label")),
                                  !sys_color);
        gtk_widget_set_sensitive (stickynotes->w_prefs_color, !sys_color);
    }
    if (g_settings_is_writable (stickynotes->settings, "default-font-color")) {
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "prefs_font_color_label")),
                                  !sys_color);
        gtk_widget_set_sensitive (stickynotes->w_prefs_font_color, !sys_color);
    }
    if (g_settings_is_writable (stickynotes->settings, "default-font")) {
        gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (stickynotes->builder,
                                                                      "prefs_font_label")),
                                  !sys_font);
        gtk_widget_set_sensitive (stickynotes->w_prefs_font, !sys_font);
    }
}

void
stickynotes_applet_update_menus (void)
{
    GList *l;
    gboolean inconsistent = FALSE;

    gboolean locked = g_settings_get_boolean (stickynotes->settings,
                                              "locked");
    gboolean locked_writable = g_settings_is_writable (stickynotes->settings,
                                                       "locked");

    for (l = stickynotes->notes; l != NULL; l = l->next) {
        StickyNote *note = l->data;

        if (note->locked != locked) {
            inconsistent = TRUE;
            break;
        }
    }

    for (l = stickynotes->applets; l != NULL; l = l->next) {
        StickyNotesApplet *applet = l->data;
        GSList *proxies, *p;

        GtkAction *action = gtk_action_group_get_action (applet->action_group,
                                                         "lock");

        g_object_set (action,
                      "active", locked,
                      "sensitive", locked_writable,
                      NULL);

        proxies = gtk_action_get_proxies (action);
        for (p = proxies; p; p = g_slist_next (p)) {
            if (GTK_IS_CHECK_MENU_ITEM (p->data))
                gtk_check_menu_item_set_inconsistent (GTK_CHECK_MENU_ITEM (p->data),
                                                      inconsistent);
        }
    }
}

void
stickynotes_applet_update_tooltips (void)
{
    int num;
    char *tooltip, *no_notes;
    StickyNotesApplet *applet;
    GList *l;

    num = g_list_length (stickynotes->notes);

    no_notes = g_strdup_printf (ngettext ("%d note",
                                          "%d notes",
                                          num),
                                num);

    tooltip = g_strdup_printf ("%s\n%s",
                               _("Show sticky notes"),
                               no_notes);

    for (l = stickynotes->applets; l; l = l->next)
    {
        applet = l->data;
        gtk_widget_set_tooltip_text (applet->w_applet,
                                     tooltip);

        if (applet->menu_tip)
            gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (applet->menu_tip))),
                                no_notes);
    }

    g_free (tooltip);
    g_free (no_notes);
}

void
stickynotes_applet_panel_icon_get_geometry (int *x,
                                            int *y,
                                            int *width,
                                            int *height)
{
    GtkWidget         *widget;
    GtkAllocation      allocation;
    GtkRequisition     requisition;
    StickyNotesApplet *applet;

    applet = stickynotes->applets->data;

    widget = GTK_WIDGET (applet->w_image);

    gtk_widget_get_preferred_size (widget,
                                   NULL, &requisition);

    gdk_window_get_origin (gtk_widget_get_window (widget), x, y);

    gtk_widget_get_allocation (widget, &allocation);
    *width = allocation.x;
    *height = allocation.y;
}
