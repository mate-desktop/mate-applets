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
#include "drive-list.h"
#include "drive-button.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE (DriveList, drive_list, GTK_TYPE_GRID);

static GVolumeMonitor *volume_monitor = NULL;

static void drive_list_finalize (GObject *object);
static void drive_list_dispose  (GObject *object);
static void drive_list_add      (GtkContainer *container, GtkWidget *child);
static void drive_list_remove   (GtkContainer *container, GtkWidget *child);

static void mount_added         (GVolumeMonitor *monitor, GMount *mount, DriveList *self);
static void mount_changed       (GVolumeMonitor *monitor, GMount *mount, DriveList *self);
static void mount_removed       (GVolumeMonitor *monitor, GMount *mount, DriveList *self);
static void volume_added        (GVolumeMonitor *monitor, GVolume *volume, DriveList *self);
static void volume_changed      (GVolumeMonitor *monitor, GVolume *volume, DriveList *self);
static void volume_removed      (GVolumeMonitor *monitor, GVolume *volume, DriveList *self);
static void add_volume          (DriveList *self, GVolume *volume);
static void remove_volume       (DriveList *self, GVolume *volume);
static void add_mount           (DriveList *self, GMount *mount);
static void remove_mount        (DriveList *self, GMount *mount);
static void queue_relayout      (DriveList *self);

static void
drive_list_class_init (DriveListClass *class)
{
    G_OBJECT_CLASS (class)->finalize = drive_list_finalize;
    G_OBJECT_CLASS (class)->dispose = drive_list_dispose;
    GTK_CONTAINER_CLASS (class)->add = drive_list_add;
    GTK_CONTAINER_CLASS (class)->remove = drive_list_remove;
}

static void
drive_list_init (DriveList *self)
{
    GList *volumes, *mounts, *tmp;

    gtk_grid_set_column_homogeneous (GTK_GRID (self), TRUE);
    gtk_grid_set_row_homogeneous (GTK_GRID (self), TRUE);

    self->volumes = g_hash_table_new (NULL, NULL);
    self->mounts = g_hash_table_new (NULL, NULL);
    self->orientation = GTK_ORIENTATION_HORIZONTAL;
    self->layout_tag = 0;
    self->settings = g_settings_new ("org.mate.drivemount");
    self->icon_size = 24;
    self->relief = GTK_RELIEF_NORMAL;

    g_signal_connect (self->settings,
                      "changed::drivemount-checkmark-color",
                      G_CALLBACK (settings_color_changed),
                      self);

    /* listen for drive connects/disconnects, and add
     * currently connected drives. */
    self->count = 0;
    if (!volume_monitor)
        volume_monitor = g_volume_monitor_get ();

    g_signal_connect_object (volume_monitor, "mount_added",
                             G_CALLBACK (mount_added), self, 0);
    g_signal_connect_object (volume_monitor, "mount_changed",
                             G_CALLBACK (mount_changed), self, 0);
    g_signal_connect_object (volume_monitor, "mount_removed",
                             G_CALLBACK (mount_removed), self, 0);
    g_signal_connect_object (volume_monitor, "volume_added",
                             G_CALLBACK (volume_added), self, 0);
    g_signal_connect_object (volume_monitor, "volume_changed",
                             G_CALLBACK (volume_changed), self, 0);
    g_signal_connect_object (volume_monitor, "volume_removed",
                             G_CALLBACK (volume_removed), self, 0);
    volumes = g_volume_monitor_get_volumes (volume_monitor);
    for (tmp = volumes; tmp != NULL; tmp = tmp->next) {
        GVolume *volume = tmp->data;
        add_volume (self, volume);
        g_object_unref (volume);
        self->count++;
    }
    g_list_free (volumes);

    mounts = g_volume_monitor_get_mounts (volume_monitor);
    for (tmp = mounts; tmp != NULL; tmp = tmp->next) {
        GMount *mount = tmp->data;
        add_mount (self, mount);
        g_object_unref (mount);
        self->count++;
    }
    self->dummy = drive_button_new (NULL);
    gtk_button_set_relief (GTK_BUTTON (self->dummy), self->relief);
    drive_button_set_size (DRIVE_BUTTON (self->dummy), self->icon_size);

    if (self->count == 0) {
        gtk_container_add (GTK_CONTAINER (self), self->dummy);
        queue_relayout (self);
        drive_button_queue_update (DRIVE_BUTTON (self->dummy));
    }
    g_list_free (mounts);
}

GtkWidget *
drive_list_new (void)
{
    return g_object_new (DRIVE_TYPE_LIST, NULL);
}

static void
drive_list_finalize (GObject *object)
{
    DriveList *self = DRIVE_LIST (object);

    g_hash_table_destroy (self->volumes);
    g_hash_table_destroy (self->mounts);
    g_object_unref (self->settings);

    if (G_OBJECT_CLASS (drive_list_parent_class)->finalize)
        (* G_OBJECT_CLASS (drive_list_parent_class)->finalize) (object);
}

static void
drive_list_dispose (GObject *object)
{
    DriveList *self = DRIVE_LIST (object);

    g_signal_handlers_disconnect_by_func (volume_monitor,
                                          G_CALLBACK (mount_added), self);
    g_signal_handlers_disconnect_by_func (volume_monitor,
                                          G_CALLBACK (mount_changed), self);
    g_signal_handlers_disconnect_by_func (volume_monitor,
                                          G_CALLBACK (mount_removed), self);
    g_signal_handlers_disconnect_by_func (volume_monitor,
                                          G_CALLBACK (volume_added), self);
    g_signal_handlers_disconnect_by_func (volume_monitor,
                                          G_CALLBACK (volume_changed), self);
    g_signal_handlers_disconnect_by_func (volume_monitor,
                                          G_CALLBACK (volume_removed), self);

    if (self->layout_tag)
        g_source_remove (self->layout_tag);
    self->layout_tag = 0;

    if (G_OBJECT_CLASS (drive_list_parent_class)->dispose)
        (* G_OBJECT_CLASS (drive_list_parent_class)->dispose) (object);
}

static void
drive_list_add (GtkContainer *container,
                GtkWidget    *child)
{
    DriveList *self;
    DriveButton *button;

    g_return_if_fail (DRIVE_IS_LIST (container));
    g_return_if_fail (DRIVE_IS_BUTTON (child));

    if (GTK_CONTAINER_CLASS (drive_list_parent_class)->add)
        (* GTK_CONTAINER_CLASS (drive_list_parent_class)->add) (container, child);

    self = DRIVE_LIST (container);
    button = DRIVE_BUTTON (child);
    if (button->volume)
        g_hash_table_insert (self->volumes, button->volume, button);
    else
        g_hash_table_insert (self->mounts, button->mount, button);
}

static void
drive_list_remove (GtkContainer *container,
                   GtkWidget    *child)
{
    DriveList *self;
    DriveButton *button;

    g_return_if_fail (DRIVE_IS_LIST (container));
    g_return_if_fail (DRIVE_IS_BUTTON (child));

    self = DRIVE_LIST (container);
    button = DRIVE_BUTTON (child);
    if (button->volume)
        g_hash_table_remove (self->volumes, button->volume);
    else
        g_hash_table_remove (self->mounts, button->mount);

    if (GTK_CONTAINER_CLASS (drive_list_parent_class)->remove)
        (* GTK_CONTAINER_CLASS (drive_list_parent_class)->remove) (container, child);
}

static void
list_buttons (gpointer key,
              gpointer value,
              gpointer user_data)
{
    GtkWidget *button = value;
    GList **sorted_buttons = user_data;

    *sorted_buttons = g_list_insert_sorted (*sorted_buttons, button,
                                            (GCompareFunc)drive_button_compare);
}

static gboolean
relayout_buttons (gpointer data)
{
    DriveList *self = DRIVE_LIST (data);
    GList *sorted_buttons = NULL, *tmp;
    int i = 0;

    self->layout_tag = 0;
    if ( self->count > 0 ) {
        g_hash_table_foreach (self->volumes, list_buttons, &sorted_buttons);
        g_hash_table_foreach (self->mounts, list_buttons, &sorted_buttons);

        /* position buttons in the table according to their sorted order */
        for (tmp = sorted_buttons, i = 0; tmp != NULL; tmp = tmp->next, i++) {
            GtkWidget *button = tmp->data;

            if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
                gtk_container_child_set (GTK_CONTAINER (self), button,
                                         "left-attach", i + 1, "top-attach", 0,
                                         "width", 1, "height", 1,
                                         NULL);
            }
            else {
                gtk_container_child_set (GTK_CONTAINER (self), button,
                                         "left-attach", 0, "top-attach", i + 1,
                                         "width", 1, "height", 1,
                                         NULL);
            }
        }
    }
    else {
        gtk_widget_show (self->dummy);
        if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
            gtk_container_child_set (GTK_CONTAINER (self), self->dummy,
                                     "left-attach", i + 1, "top-attach", 0,
                                     "width", 1, "height", 1,
                                     NULL);
        }
        else {
            gtk_container_child_set (GTK_CONTAINER (self), self->dummy,
                                     "left-attach", 0, "top-attach", i + 1,
                                     "width", 1, "height", 1,
                                     NULL);
        }
    }
    return FALSE;
}

static void
queue_relayout (DriveList *self)
{
    if (!self->layout_tag) {
        self->layout_tag = g_idle_add (relayout_buttons, self);
    }
}

static void
mount_added (GVolumeMonitor *monitor,
             GMount         *mount,
             DriveList      *self)
{
    add_mount (self, mount);
    self->count++;
    if (self->count == 1)
        gtk_container_remove (GTK_CONTAINER (self), g_object_ref (self->dummy));

    mount_changed (monitor, mount, self);
}

static void
mount_changed (GVolumeMonitor *monitor,
               GMount         *mount,
               DriveList      *self)
{
    GVolume *volume;
    DriveButton *button = NULL;;

    volume = g_mount_get_volume (mount);
    if (volume) {
        button = g_hash_table_lookup (self->volumes, volume);
        g_object_unref (volume);
    } else {
        button = g_hash_table_lookup (self->mounts, mount);
    }
    if (button)
        drive_button_queue_update (button);
}

static void
mount_removed (GVolumeMonitor *monitor,
               GMount         *mount,
               DriveList      *self)
{
    remove_mount (self, mount);
    mount_changed (monitor, mount, self);
    self->count--;
    if (self->count == 0) {
        gtk_container_add (GTK_CONTAINER (self), self->dummy);
        queue_relayout (self);
    }
}

static void
volume_added (GVolumeMonitor *monitor,
              GVolume        *volume,
              DriveList      *self)
{
    add_volume (self, volume);
    self->count++;
    if (self->count == 1)
        gtk_container_remove (GTK_CONTAINER (self), g_object_ref (self->dummy));
}

static void
volume_changed (GVolumeMonitor *monitor,
                GVolume        *volume,
                DriveList      *self)
{
    DriveButton *button = NULL;;

    button = g_hash_table_lookup (self->volumes, volume);
    if (button)
        drive_button_queue_update (button);
}

static void
volume_removed (GVolumeMonitor *monitor,
                GVolume        *volume,
                DriveList      *self)
{
    remove_volume (self, volume);
    self->count--;
    if (self->count == 0) {
        gtk_container_add (GTK_CONTAINER (self), self->dummy);
        queue_relayout (self);
    }
}

static void
add_volume (DriveList *self,
            GVolume   *volume)
{
    GtkWidget *button;

    /* if the volume has already been added, return */
    if (g_hash_table_lookup (self->volumes, volume) != NULL)
        return;

    button = drive_button_new (volume);
    gtk_button_set_relief (GTK_BUTTON (button), self->relief);
    drive_button_set_size (DRIVE_BUTTON (button), self->icon_size);
    gtk_container_add (GTK_CONTAINER (self), button);
    gtk_widget_show (button);
    queue_relayout (self);
}

static void
remove_volume (DriveList *self,
               GVolume   *volume)
{
    GtkWidget *button;

    /* if the volume has already been added, return */
    button = g_hash_table_lookup (self->volumes, volume);
    if (button) {
        gtk_container_remove (GTK_CONTAINER (self), button);
        queue_relayout (self);
    }
}

static void
add_mount (DriveList *self,
           GMount    *mount)
{
    GtkWidget *button;
    GVolume *volume;

    /* ignore mounts reported as shadowed */
    if (g_mount_is_shadowed (mount)) {
        return;
    }

    /* ignore mounts attached to a volume */
    volume = g_mount_get_volume (mount);
    if (volume) {
        g_object_unref (volume);
        return;
    }

    /* if the mount has already been added, return */
    if (g_hash_table_lookup (self->mounts, mount) != NULL)
        return;

    button = drive_button_new_from_mount (mount);
    gtk_button_set_relief (GTK_BUTTON (button), self->relief);
    drive_button_set_size (DRIVE_BUTTON (button), self->icon_size);
    gtk_container_add (GTK_CONTAINER (self), button);
    gtk_widget_show (button);
    queue_relayout (self);
}

static void
remove_mount (DriveList *self,
              GMount    *mount)
{
    GtkWidget *button;

    /* if the mount has already been added, return */
    button = g_hash_table_lookup (self->mounts, mount);
    if (button) {
        gtk_container_remove (GTK_CONTAINER (self), button);
        queue_relayout (self);
    }
}

void
drive_list_set_orientation (DriveList      *self,
                            GtkOrientation  orientation)
{
    g_return_if_fail (DRIVE_IS_LIST (self));

    if (orientation != self->orientation) {
        self->orientation = orientation;
        queue_relayout (self);
    }
}

static void
set_icon_size (gpointer key,
               gpointer value,
               gpointer user_data)
{
    DriveButton *button = value;
    DriveList *self = user_data;

    drive_button_set_size (button, self->icon_size);
}


void
drive_list_set_panel_size (DriveList *self,
                           int        panel_size)
{
    g_return_if_fail (DRIVE_IS_LIST (self));

    if (self->icon_size != panel_size) {
        self->icon_size = panel_size;
        g_hash_table_foreach (self->volumes, set_icon_size, self);
        g_hash_table_foreach (self->mounts, set_icon_size, self);
    }
}

void
drive_list_redraw (DriveList *self)
{
    g_hash_table_foreach (self->volumes, drive_button_redraw, self);
    g_hash_table_foreach (self->mounts, drive_button_redraw, self);
}

void
settings_color_changed (GSettings *settings,
                        gchar     *key,
                        DriveList *drive_list)
{
    g_return_if_fail (DRIVE_IS_LIST (drive_list));
    drive_list_redraw (drive_list);
}

static void
set_button_relief (gpointer key,
                   gpointer value,
                   gpointer user_data)
{
    GtkButton *button = value;
    DriveList *self = user_data;

    gtk_button_set_relief (button, self->relief);
}

void
drive_list_set_transparent (DriveList *self,
                            gboolean   transparent)
{
    GtkReliefStyle relief;

    relief  = transparent ? GTK_RELIEF_NONE : GTK_RELIEF_NORMAL;

    if (relief == self->relief)
        return;

    self->relief = relief;
    g_hash_table_foreach (self->volumes, set_button_relief, self);
    g_hash_table_foreach (self->mounts, set_button_relief, self);
}
