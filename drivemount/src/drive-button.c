/* -*- mode: C; c-basic-offset: 4 -*-
 * Drive Mount Applet
 * Copyright (c) 2004 Canonical Ltd
 * Copyright 2008 Pierre Ossman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author:
 *   James Henstridge <jamesh@canonical.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include "drive-button.h"
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gdesktopappinfo.h>

#include <string.h>

enum {
    CMD_NONE,
    CMD_MOUNT_OR_PLAY,
    CMD_UNMOUNT,
    CMD_EJECT
};

/* type registration boilerplate code */
G_DEFINE_TYPE (DriveButton, drive_button, GTK_TYPE_BUTTON)

static void     drive_button_set_volume   (DriveButton    *self,
                                           GVolume        *volume);
static void     drive_button_set_mount    (DriveButton    *self,
                                           GMount         *mount);
static void     drive_button_reset_popup  (DriveButton    *self);
static void     drive_button_ensure_popup (DriveButton    *self);

static void     drive_button_dispose      (GObject        *object);
#if 0
static void     drive_button_unrealize    (GtkWidget      *widget);
#endif /* 0 */
static gboolean drive_button_button_press (GtkWidget      *widget,
                                           GdkEventButton *event);
static gboolean drive_button_key_press    (GtkWidget      *widget,
                                           GdkEventKey    *event);
static void drive_button_theme_change     (GtkIconTheme   *icon_theme,
                                           gpointer        data);

static void
drive_button_class_init (DriveButtonClass *class)
{
    G_OBJECT_CLASS (class)->dispose = drive_button_dispose;
    GTK_WIDGET_CLASS (class)->button_press_event = drive_button_button_press;
    GTK_WIDGET_CLASS (class)->key_press_event = drive_button_key_press;

    GtkCssProvider *provider;

    provider = gtk_css_provider_new ();

    gtk_css_provider_load_from_data (provider,
                                     "#drive-button {\n"
                                     " border-width: 0px;\n"
                                     " padding: 0px;\n"
                                     " margin: 0px;\n"
                                     "}",
                                     -1, NULL);

    gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                    GTK_STYLE_PROVIDER (provider),
                                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (provider);
}

static void
drive_button_init (DriveButton *self)
{
    GtkWidget *image;

    image = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (self), image);
    gtk_widget_show (image);

    self->volume = NULL;
    self->mount = NULL;
    self->icon_size = 24;
    self->update_tag = 0;

    self->popup_menu = NULL;

    gtk_widget_set_name (GTK_WIDGET (self), "drive-button");
}

GtkWidget *
drive_button_new (GVolume *volume)
{
    DriveButton *self;

    self = g_object_new (DRIVE_TYPE_BUTTON, NULL);
    if (volume != NULL) {
      drive_button_set_volume (self, volume);
      g_signal_connect (gtk_icon_theme_get_default (),
          "changed", G_CALLBACK (drive_button_theme_change),
          self);
    }

    return (GtkWidget *)self;
}

GtkWidget *
drive_button_new_from_mount (GMount *mount)
{
    DriveButton *self;

    self = g_object_new (DRIVE_TYPE_BUTTON, NULL);
    drive_button_set_mount (self, mount);

    g_signal_connect (gtk_icon_theme_get_default (),
        "changed", G_CALLBACK (drive_button_theme_change),
        self);

    return (GtkWidget *)self;
}

static void
drive_button_dispose (GObject *object)
{
    DriveButton *self = DRIVE_BUTTON (object);

    drive_button_set_volume (self, NULL);

    if (self->update_tag)
        g_source_remove (self->update_tag);
    self->update_tag = 0;

    drive_button_reset_popup (self);

    if (G_OBJECT_CLASS (drive_button_parent_class)->dispose)
        (* G_OBJECT_CLASS (drive_button_parent_class)->dispose) (object);
}

static gboolean
drive_button_button_press (GtkWidget      *widget,
                           GdkEventButton *event)
{
    DriveButton *self = DRIVE_BUTTON (widget);

    /* don't consume non-button1 presses */
    if (event->button == 1) {
        drive_button_ensure_popup (self);
        if (self->popup_menu) {
            gtk_menu_popup_at_widget (GTK_MENU (self->popup_menu),
                                      widget,
                                      GDK_GRAVITY_SOUTH_WEST,
                                      GDK_GRAVITY_NORTH_WEST,
                                      (const GdkEvent*) event);
        }
        return TRUE;
    }
    return FALSE;
}

static gboolean
drive_button_key_press (GtkWidget      *widget,
                        GdkEventKey    *event)
{
    DriveButton *self = DRIVE_BUTTON (widget);

    switch (event->keyval) {
    case GDK_KEY_KP_Space:
    case GDK_KEY_space:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_Return:
        drive_button_ensure_popup (self);
        if (self->popup_menu) {
            gtk_menu_popup_at_widget (GTK_MENU (self->popup_menu),
                                      widget,
                                      GDK_GRAVITY_SOUTH_WEST,
                                      GDK_GRAVITY_NORTH_WEST,
                                      (const GdkEvent*) event);
        }
        return TRUE;
    }
    return FALSE;
}

static void
drive_button_theme_change (GtkIconTheme *icon_theme,
                           gpointer      data)
{
    drive_button_queue_update (data);
}

static void
drive_button_set_volume (DriveButton *self,
                         GVolume     *volume)
{
    g_return_if_fail (DRIVE_IS_BUTTON (self));

    if (self->volume) {
        g_object_unref (self->volume);
    }
    self->volume = NULL;
    if (self->mount) {
        g_object_unref (self->mount);
    }
    self->mount = NULL;

    if (volume) {
        self->volume = g_object_ref (volume);
    }
    drive_button_queue_update (self);
}

static void
drive_button_set_mount (DriveButton *self,
                        GMount       *mount)
{
    g_return_if_fail (DRIVE_IS_BUTTON (self));

    if (self->volume) {
        g_object_unref (self->volume);
    }
    self->volume = NULL;
    if (self->mount) {
        g_object_unref (self->mount);
    }
    self->mount = NULL;

    if (mount) {
        self->mount = g_object_ref (mount);
    }
    drive_button_queue_update (self);
}

static gboolean
drive_button_update (gpointer user_data)
{
    DriveButton *self;
    GdkScreen *screen;
    GtkIconTheme *icon_theme;
    GtkIconInfo *icon_info;
    GIcon *icon;
    int width, height, scale;
    cairo_t *cr;
    cairo_surface_t *surface = NULL;
    cairo_surface_t *tmp_surface = NULL;
    GtkRequisition button_req, image_req;
    char *display_name, *tip;

    g_return_val_if_fail (DRIVE_IS_BUTTON (user_data), FALSE);
    self = DRIVE_BUTTON (user_data);
    self->update_tag = 0;

    /* base the icon size on the desired button size */
    drive_button_reset_popup (self);
    scale = gtk_widget_get_scale_factor (GTK_WIDGET (self));
    gtk_widget_get_preferred_size (GTK_WIDGET (self), NULL, &button_req);
    gtk_widget_get_preferred_size (gtk_bin_get_child (GTK_BIN (self)), NULL, &image_req);
    width = (self->icon_size - (button_req.width - image_req.width)) / scale;
    height = (self->icon_size - (button_req.height - image_req.height)) / scale;

    /* if no volume or mount, display general image */
    if (!self->volume && !self->mount)
    {
        gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("nothing to mount"));
        screen = gtk_widget_get_screen (GTK_WIDGET (self));
        icon_theme = gtk_icon_theme_get_for_screen (screen); //m
        // note - other good icon would be emblem-unreadable
        icon_info = gtk_icon_theme_lookup_icon_for_scale (icon_theme, "media-floppy",
                                                          MIN (width, height), scale,
                                                          GTK_ICON_LOOKUP_USE_BUILTIN);
        if (icon_info) {
            surface = gtk_icon_info_load_surface (icon_info, NULL, NULL);
            g_object_unref (icon_info);
        }

        if (!surface)
            return FALSE;

        if (gtk_bin_get_child (GTK_BIN (self)) != NULL)
            gtk_image_set_from_surface (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (self))), surface);

        return FALSE;
    }

    gboolean is_mounted = FALSE;

    if (self->volume)
    {
        GMount *mount;

        display_name = g_volume_get_name (self->volume);
        mount = g_volume_get_mount (self->volume);

        if (mount)
        {
            is_mounted = TRUE;
            tip = g_strdup_printf ("%s\n%s", display_name, _("(mounted)"));
            icon = g_mount_get_icon (mount);
            g_object_unref (mount);
        }
        else
        {
            is_mounted = FALSE;
            tip = g_strdup_printf ("%s\n%s", display_name, _("(not mounted)"));
            icon = g_volume_get_icon (self->volume);
        }
    } else
    {
        is_mounted = TRUE;
        display_name = g_mount_get_name (self->mount);
        tip = g_strdup_printf ("%s\n%s", display_name, _("(mounted)"));
        icon = g_mount_get_icon (self->mount);
    }

    gtk_widget_set_tooltip_text (GTK_WIDGET (self), tip);
    g_free (tip);
    g_free (display_name);

    screen = gtk_widget_get_screen (GTK_WIDGET (self));
    icon_theme = gtk_icon_theme_get_for_screen (screen);
    icon_info = gtk_icon_theme_lookup_by_gicon_for_scale (icon_theme, icon,
                                                          MIN (width, height), scale,
                                                          GTK_ICON_LOOKUP_USE_BUILTIN);
    if (icon_info)
    {
        surface = gtk_icon_info_load_surface (icon_info, NULL, NULL);
        g_object_unref (icon_info);
    }

    g_object_unref (icon);

    if (!surface)
        return FALSE;

    // create a new surface because icon image can be shared by system
    tmp_surface = cairo_surface_create_similar (surface,
                                                cairo_surface_get_content (surface),
                                                cairo_image_surface_get_width (surface) / scale,
                                                cairo_image_surface_get_height (surface) / scale);

    // if mounted, change icon
    if (is_mounted)
    {
        int icon_width, icon_height, rowstride, n_channels, x, y;
        guchar *pixels, *p;
        gboolean has_alpha;

        has_alpha = cairo_surface_get_content (tmp_surface) != CAIRO_CONTENT_COLOR;
        n_channels = 3;
        if (has_alpha)
            n_channels++;

        icon_width = cairo_image_surface_get_width (tmp_surface);
        icon_height = cairo_image_surface_get_height (tmp_surface);

        rowstride = cairo_image_surface_get_stride (tmp_surface);
        pixels = cairo_image_surface_get_data (tmp_surface);

        GdkRGBA color;
        GSettings *settings;
        settings = g_settings_new ("org.mate.drivemount");
        gchar *color_string = g_settings_get_string (settings, "drivemount-checkmark-color");
        if (!color_string)
                color_string = g_strdup ("#00ff00");
        gdk_rgba_parse (&color, color_string);
        g_free (color_string);
        g_object_unref (settings);

        guchar red = color.red*255;
        guchar green = color.green*255;
        guchar blue = color.blue*255;

        const gdouble ratio = 0.65;
        gdouble y_start = icon_height * ratio;
        gdouble x_start = icon_height * (1 + ratio);

        for (y = y_start; y < icon_height; y++)
            for (x = x_start - y; x < icon_width; x++)
            {
                p = pixels + y * rowstride + x * n_channels;
                p[0] = red;
                p[1] = green;
                p[2] = blue;
                if (has_alpha)
                    p[3] = 255;
            }
    }

    cr = cairo_create (tmp_surface);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVERLAY);
    cairo_set_source_surface (cr, surface, 0, 0);
    cairo_paint (cr);

    gtk_image_set_from_surface (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (self))), tmp_surface);

    cairo_surface_destroy (surface);
    cairo_surface_destroy (tmp_surface);

    gtk_widget_get_preferred_size (GTK_WIDGET (self), NULL, &button_req);

    return FALSE;
}

void
drive_button_queue_update (DriveButton *self)
{
    if (!self->update_tag) {
        self->update_tag = g_idle_add (drive_button_update, self);
    }
}

void
drive_button_set_size (DriveButton *self,
                       int icon_size)
{
    g_return_if_fail (DRIVE_IS_BUTTON (self));

    if (self->icon_size != icon_size) {
        self->icon_size = icon_size;
        drive_button_queue_update (self);
    }
}

void
drive_button_redraw (gpointer key,
                     gpointer value,
                     gpointer user_data)
{
    DriveButton *button = value;
    drive_button_queue_update (button);
}

int
drive_button_compare (DriveButton *button,
                      DriveButton *other_button)
{
    /* sort drives before driveless volumes volumes */
    if (button->volume) {
        if (other_button->volume) {
            int cmp;
            gchar *str1, *str2;

            str1 = g_volume_get_name (button->volume);
            str2 = g_volume_get_name (other_button->volume);
            cmp = g_utf8_collate (str1, str2);
            g_free (str2);
            g_free (str1);

            return cmp;
        } else {
            return -1;
        }
    } else {
        if (other_button->volume) {
            return 1;
        } else {
            int cmp;
            gchar *str1, *str2;

            str1 = g_mount_get_name (button->mount);
            str2 = g_mount_get_name (other_button->mount);
            cmp = g_utf8_collate (str1, str2);
            g_free (str2);
            g_free (str1);

            return cmp;
        }
    }
}

static void
drive_button_reset_popup (DriveButton *self)
{
    if (self->popup_menu)
        gtk_widget_destroy (GTK_WIDGET (self->popup_menu));
    self->popup_menu = NULL;
}

#if 0
static void
popup_menu_detach (GtkWidget *attach_widget, GtkMenu *menu)
{
    DRIVE_BUTTON (attach_widget)->popup_menu = NULL;
}
#endif /* 0 */

static char *
escape_underscores (const char *str)
{
    char *new_str;
    int i, j, count;

    /* count up how many underscores are in the string */
    count = 0;
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == '_')
            count++;
    }
    /* copy to new string, doubling up underscores */
    new_str = g_new (char, i + count + 1);
    for (i = j = 0; str[i] != '\0'; i++, j++) {
        new_str[j] = str[i];
        if (str[i] == '_')
            new_str[++j] = '_';
    }
    new_str[j] = '\0';
    return new_str;
}
static GtkWidget *
create_menu_item (DriveButton *self,
                  const gchar *icon_name,
                  const gchar *label,
                  GCallback    callback,
                  gboolean     sensitive)
{
    GtkWidget *item, *image;

    item = gtk_image_menu_item_new_with_mnemonic (label);
    if (icon_name) {
        image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
        gtk_widget_show (image);
    }
    if (callback)
        g_signal_connect_object (item, "activate", callback, self,
                                 G_CONNECT_SWAPPED);
    gtk_widget_set_sensitive (item, sensitive);
    gtk_widget_show (item);
    return item;
}

static void
open_drive (DriveButton *self,
            GtkWidget   *item)
{
    GdkScreen *screen;
    GtkWidget *dialog;
    GError *error = NULL;
    GFile *file = NULL;
    GList *files = NULL;
    GdkAppLaunchContext *launch_context;
    GAppInfo *app_info;

    if (self->volume) {
        GMount *mount;

        mount = g_volume_get_mount (self->volume);
        if (mount) {
            file = g_mount_get_root (mount);
            g_object_unref (mount);
        }
    } else if (self->mount) {
        file = g_mount_get_root (self->mount);
    } else
        g_return_if_reached ();

    app_info = g_app_info_get_default_for_type ("inode/directory", FALSE);
    if (!app_info)
      app_info = G_APP_INFO (g_desktop_app_info_new ("caja.desktop"));

    if (app_info) {
        GdkDisplay *display = gtk_widget_get_display (item);
        launch_context = gdk_display_get_app_launch_context (display);
        screen = gtk_widget_get_screen (GTK_WIDGET (self));
        gdk_app_launch_context_set_screen (launch_context, screen);
        files = g_list_prepend (files, file);
        g_app_info_launch (app_info,
        files,
        G_APP_LAUNCH_CONTEXT (launch_context),
        &error);

        g_object_unref (launch_context);
        g_list_free (files);
    }

    if (!app_info || error) {
        dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))),
                                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     _("Cannot execute Caja"));
        if (error)
            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
        else
            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "Could not find Caja");
        g_signal_connect (dialog, "response",
                          G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_widget_show (dialog);
        g_error_free (error);
    }

    g_object_unref (file);
}

/* copied from mate-volume-manager/src/manager.c maybe there is a better way than
 * duplicating this code? */

/*
 * gvm_run_command - run the given command, replacing %d with the device node
 * and %m with the given path
 */
static void
gvm_run_command (const char *device,
                 const char *command,
                 const char *path)
{
    char *argv[4];
    gchar *new_command;
    GError *error = NULL;
    GString *exec = g_string_new (NULL);
    char *p, *q;

    /* perform s/%d/device/ and s/%m/path/ */
    new_command = g_strdup (command);
    q = new_command;
    p = new_command;
    while ((p = strchr (p, '%')) != NULL) {
        if (*(p + 1) == 'd') {
            *p = '\0';
            g_string_append (exec, q);
            g_string_append (exec, device);
            q = p + 2;
            p = p + 2;
        } else if (*(p + 1) == 'm') {
            *p = '\0';
            g_string_append (exec, q);
            g_string_append (exec, path);
            q = p + 2;
            p = p + 2;
        } else {
            /* Ignore anything else. */
            p++;
        }
    }
    g_string_append (exec, q);

    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = exec->str;
    argv[3] = NULL;

    g_spawn_async (g_get_home_dir (), argv, NULL, 0, NULL, NULL,
                   NULL, &error);
    if (error) {
        g_warning ("failed to exec %s: %s\n", exec->str, error->message);
        g_error_free (error);
    }

    g_string_free (exec, TRUE);
    g_free (new_command);
}

/*
 * gvm_check_dvd_only - is this a Video DVD?
 *
 * Returns TRUE if this was a Video DVD and FALSE otherwise.
 * (the original in gvm was also running the autoplay action,
 * I removed that code, so I renamed from gvm_check_dvd to
 * gvm_check_dvd_only)
 */
static gboolean
gvm_check_dvd_only (const char *udi,
                    const char *device,
                    const char *mount_point)
{
    char *path;
    gboolean retval;

    path = g_build_path (G_DIR_SEPARATOR_S, mount_point, "video_ts", NULL);
    retval = g_file_test (path, G_FILE_TEST_IS_DIR);
    g_free (path);

    /* try the other name, if needed */
    if (retval == FALSE) {
        path = g_build_path (G_DIR_SEPARATOR_S, mount_point,
                             "VIDEO_TS", NULL);
        retval = g_file_test (path, G_FILE_TEST_IS_DIR);
        g_free (path);
    }

    return retval;
}
/* END copied from mate-volume-manager/src/manager.c */

static gboolean
check_dvd_video (DriveButton *self)
{
    GFile *file;
    char *udi, *device_path, *mount_path;
    gboolean result;
    GMount *mount;

    if (!self->volume)
        return FALSE;

    mount = g_volume_get_mount (self->volume);
    if (!mount)
        return FALSE;

    file = g_mount_get_root (mount);
    g_object_unref (mount);

    if (!file)
        return FALSE;

    mount_path = g_file_get_path (file);

    g_object_unref (file);

    device_path = g_volume_get_identifier (self->volume,
                                           G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    udi = g_volume_get_identifier (self->volume,
                                   G_VOLUME_IDENTIFIER_KIND_HAL_UDI);

    result = gvm_check_dvd_only (udi, device_path, mount_path);

    g_free (device_path);
    g_free (udi);
    g_free (mount_path);

    return result;
}

static gboolean
check_audio_cd (DriveButton *self)
{
    GFile *file;
    char *activation_uri;
    GMount *mount;

    if (!self->volume)
        return FALSE;

    mount = g_volume_get_mount (self->volume);
    if (!mount)
        return FALSE;

    file = g_mount_get_root (mount);
    g_object_unref (mount);

    if (!file)
        return FALSE;

    activation_uri = g_file_get_uri (file);

    g_object_unref (file);

    /* we have an audioCD if the activation URI starts by 'cdda://' */
    gboolean result = (strncmp ("cdda://", activation_uri, 7) == 0);
    g_free (activation_uri);
    return result;
}

static void
run_command (DriveButton *self,
             const char  *command)
{
    GFile *file;
    char *mount_path, *device_path;
    GMount *mount;

    if (!self->volume)
        return;

    mount = g_volume_get_mount (self->volume);
    if (!mount)
        return;

    file = g_mount_get_root (mount);
    g_object_unref (mount);

    g_assert (file);

    mount_path = g_file_get_path (file);

    g_object_unref (file);

    device_path = g_volume_get_identifier (self->volume,
                                           G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);

    gvm_run_command (device_path, command, mount_path);

    g_free (mount_path);
    g_free (device_path);
}

static void dummy_async_ready_callback (GObject      *source_object,
                                        GAsyncResult *res,
                                        gpointer      user_data)
{
    /* do nothing */
}

static void
mount_drive (DriveButton *self,
             GtkWidget   *item)
{
    if (self->volume) {
        GMountOperation *mount_op = gtk_mount_operation_new (NULL);
        g_volume_mount (self->volume, G_MOUNT_MOUNT_NONE,
                        mount_op, NULL, dummy_async_ready_callback, NULL);
        g_object_unref (mount_op);
    } else {
        g_return_if_reached ();
    }
}

static void
unmount_drive (DriveButton *self,
               GtkWidget   *item)
{
    if (self->volume) {
        GMount *mount;

        mount = g_volume_get_mount (self->volume);
        if (mount) {
            g_mount_unmount_with_operation (mount, G_MOUNT_UNMOUNT_NONE,
                                            NULL, NULL, dummy_async_ready_callback, NULL);
            g_object_unref (mount);
        }
    } else if (self->mount) {
        g_mount_unmount_with_operation (self->mount, G_MOUNT_UNMOUNT_NONE,
                                        NULL, NULL, dummy_async_ready_callback, NULL);
    } else {
        g_return_if_reached ();
    }
}

static void eject_finish (DriveButton  *self,
                          GAsyncResult *res,
                          gpointer      user_data)
{
    /* Do nothing. We shouldn't need this according to the GIO
     * docs, but the applet crashes without it using glib 2.18.0 */
}

static void
eject_drive (DriveButton *self,
             GtkWidget   *item)
{
    if (self->volume) {
        g_volume_eject_with_operation (self->volume, G_MOUNT_UNMOUNT_NONE,
                                       NULL, NULL,
                                       (GAsyncReadyCallback) eject_finish,
                                       NULL);
    } else if (self->mount) {
        g_mount_eject_with_operation (self->mount, G_MOUNT_UNMOUNT_NONE,
                                      NULL, NULL,
                                      (GAsyncReadyCallback) eject_finish,
                                      NULL);
    } else {
        g_return_if_reached ();
    }
}
static void
play_autoplay_media (DriveButton *self,
                     const char  *dflt)
{
    run_command (self, dflt);
}

static void
play_dvd (DriveButton *self,
          GtkWidget   *item)
{
    /* FIXME add an option to set this */
    play_autoplay_media (self, "totem %d");
}

static void
play_cda (DriveButton *self,
          GtkWidget   *item)
{
    /* FIXME add an option to set this */
    play_autoplay_media (self, "sound-juicer -d %d");
}

static void
drive_button_ensure_popup (DriveButton *self)
{
    char *display_name, *tmp, *label;
    GtkWidget *item;
    gboolean mounted, ejectable;

    if (self->popup_menu) return;

    mounted = FALSE;

    if (self->volume) {
        GMount *mount = NULL;

        display_name = g_volume_get_name (self->volume);
        ejectable = g_volume_can_eject (self->volume);

        mount = g_volume_get_mount (self->volume);
        if (mount) {
            mounted = TRUE;
            g_object_unref (mount);
        }
    } else {
        if (!G_IS_MOUNT (self->volume))
            return;
        else {
            display_name = g_mount_get_name (self->mount);
            ejectable = g_mount_can_eject (self->mount);
            mounted = TRUE;
        }
    }

    self->popup_menu = gtk_menu_new ();

    /* make sure the display name doesn't look like a mnemonic */
    tmp = escape_underscores (display_name ? display_name : "(none)");
    g_free (display_name);
    display_name = tmp;

    if (check_dvd_video (self)) {
        item = create_menu_item (self, "media-playback-start",
                                 _("_Play DVD"), G_CALLBACK (play_dvd),
                                 TRUE);
    } else if (check_audio_cd (self)) {
        item = create_menu_item (self, "media-playback-start",
                                 _("_Play CD"), G_CALLBACK (play_cda),
                                 TRUE);
    } else {
        label = g_strdup_printf (_("_Open %s"), display_name);
        item = create_menu_item (self, "document-open", label,
                                 G_CALLBACK (open_drive), mounted);
        g_free (label);
    }
    gtk_container_add (GTK_CONTAINER (self->popup_menu), item);

    if (mounted) {
        label = g_strdup_printf (_("Un_mount %s"), display_name);
        item = create_menu_item (self, NULL, label,
                                 G_CALLBACK (unmount_drive), TRUE);
        g_free (label);
        gtk_container_add (GTK_CONTAINER (self->popup_menu), item);
    } else {
        label = g_strdup_printf (_("_Mount %s"), display_name);
        item = create_menu_item (self, NULL, label,
                                 G_CALLBACK (mount_drive), TRUE);
        g_free (label);
        gtk_container_add (GTK_CONTAINER (self->popup_menu), item);
    }

    if (ejectable) {
        label = g_strdup_printf (_("_Eject %s"), display_name);
        item = create_menu_item (self, "media-eject", label,
                                 G_CALLBACK (eject_drive), TRUE);
        g_free (label);
        gtk_container_add (GTK_CONTAINER (self->popup_menu), item);
    }

    /*Set up custom theme and transparency support */
    GtkWidget *toplevel = gtk_widget_get_toplevel (self->popup_menu);
    /* Fix any failures of compiz/other wm's to communicate with gtk for transparency */
    GdkScreen *screen2 = gtk_widget_get_screen (GTK_WIDGET (toplevel));
    GdkVisual *visual = gdk_screen_get_rgba_visual (screen2);
    gtk_widget_set_visual (GTK_WIDGET (toplevel), visual);
    /*set menu and it's toplevel window to follow panel theme */
    GtkStyleContext *context;
    context = gtk_widget_get_style_context (GTK_WIDGET (toplevel));
    gtk_style_context_add_class (context,"gnome-panel-menu-bar");
    gtk_style_context_add_class (context,"mate-panel-menu-bar");
}
