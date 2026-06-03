/*
 * Copyright (C) 2005-2009 Alex Murray <murray.alex@gmail.com>
 * Copyright (C) 2012-2021 MATE Developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef PREFS_DIALOG_H
#define PREFS_DIALOG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sensors-applet.h"

typedef struct {
    SensorsApplet *sensors_applet;
    GtkDialog *dialog;
    GtkNotebook *notebook;

    /* widgets for global prefs */
    GtkSpinButton *timeout_spinbutton, *graph_size_spinbutton;
    GtkGrid *globals_grid;
    GtkHSeparator *globals_separator;
    GtkComboBoxText *display_mode_combo_box, *layout_mode_combo_box, *temperature_scale_combo_box;
    GtkLabel *timeout_label, *display_mode_label, *layout_mode_label, *temperature_scale_label, *graph_size_label, *update_header, *display_header;
    GtkAdjustment *timeout_adjust, *graph_size_adjust;
    GtkWidget *show_units;

#ifdef HAVE_LIBNOTIFY
    GtkCheckButton *display_notifications;
    GtkLabel *notifications_header;
#endif

    /* widgets for sensors tree */
    GtkTreeView *view;
    GtkTreeViewColumn *id_column, *label_column, *enable_column, *icon_column;
    GtkCellRenderer *id_renderer, *label_renderer, *enable_renderer, *icon_renderer;
    GtkScrolledWindow *scrolled_window;

    GtkButtonBox *buttons_box; /* holds sensor reorder buttons */
    GtkBox *sensors_hbox; /* holds scrolled window and buttons_vbox */
    GtkBox *sensors_vbox; /* holds sensors_hbox and sensor_config_hbox */
    GtkBox *sensor_config_hbox; /* holds config button */
    GtkSizeGroup *size_group; /* so comboboxes all request the same size */

    GtkButton *sensor_up_button;
    GtkButton *sensor_down_button;
    GtkButton *sensor_config_button;
} PrefsDialog;

/* function prototypes */
void prefs_dialog_open(SensorsApplet *sensors_applet);
void prefs_dialog_close(SensorsApplet *sensors_applet);

#endif /* PREFS_DIALOG_H */
