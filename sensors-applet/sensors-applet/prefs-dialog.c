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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib/gi18n.h>
#include <gio/gio.h>
#include "sensors-applet-settings.h"
#include "prefs-dialog.h"
#include "sensor-config-dialog.h"

#define OLD_TEMP_SCALE 0
#define NEW_TEMP_SCALE 1

/* when a user closes the prefs-dialog we assume that applet is now
   setup, so store all values in gsettings */
void prefs_dialog_close(SensorsApplet *sensors_applet) {

    if (sensors_applet->sensors != NULL) {
        sensors_applet_settings_save_sensors(sensors_applet);
    }

    if (sensors_applet->prefs_dialog) {
        gtk_widget_destroy(GTK_WIDGET(sensors_applet->prefs_dialog->dialog));
        g_free(sensors_applet->prefs_dialog);
        sensors_applet->prefs_dialog = NULL;
    }

    if (sensors_applet->timeout_id == 0) {
        sensors_applet->timeout_id = g_timeout_add_seconds(g_settings_get_int (sensors_applet->settings, TIMEOUT) / 1000, (GSourceFunc)sensors_applet_update_active_sensors, sensors_applet);
    }

}

static
void prefs_dialog_response(GtkDialog *prefs_dialog,
                           gint response,
                           gpointer data) {

    SensorsApplet *sensors_applet;
    GError *error = NULL;
    gint current_page;
    gchar *uri;

    sensors_applet = (SensorsApplet *)data;

    switch (response) {
        case GTK_RESPONSE_HELP:
            g_debug("loading help in prefs");
            current_page = gtk_notebook_get_current_page(sensors_applet->prefs_dialog->notebook);
            uri = g_strdup_printf("help:mate-sensors-applet/%s",
                          ((current_page == 0) ?
                           "sensors-applet-general-options" :
                           ((current_page == 1) ?
                        "sensors-applet-sensors" :
                        NULL)));
            gtk_show_uri_on_window(NULL, uri, gtk_get_current_event_time(), &error);
            g_free(uri);

            if (error) {
                g_debug("Could not open help document: %s ",error->message);
                g_error_free (error);
            }
            break;
        default:
            g_debug("closing prefs dialog");
            prefs_dialog_close(sensors_applet);
    }
}

static gboolean prefs_dialog_convert_low_and_high_values(GtkTreeModel *model,
                                                         GtkTreePath *path,
                                                         GtkTreeIter *iter,
                                                         TemperatureScale scales[2]) {

    SensorType sensor_type;
    gdouble low_value, high_value;

    gtk_tree_model_get(model,
                       iter,
                       SENSOR_TYPE_COLUMN, &sensor_type,
                       LOW_VALUE_COLUMN, &low_value,
                       HIGH_VALUE_COLUMN, &high_value,
                       -1);

    if (sensor_type == TEMP_SENSOR) {
        low_value = sensors_applet_convert_temperature(low_value,
                                                       scales[OLD_TEMP_SCALE],
                                                       scales[NEW_TEMP_SCALE]);

        high_value = sensors_applet_convert_temperature(high_value,
                                                        scales[OLD_TEMP_SCALE],
                                                        scales[NEW_TEMP_SCALE]);

        gtk_tree_store_set(GTK_TREE_STORE(model),
                           iter,
                           LOW_VALUE_COLUMN, low_value,
                           HIGH_VALUE_COLUMN, high_value,
                           -1);
    }
    return FALSE;
}

static void prefs_dialog_timeout_changed(GtkSpinButton *button,
                                         PrefsDialog *prefs_dialog) {

    gint value;
    value = (gint)(gtk_spin_button_get_value(button) * 1000);
    g_settings_set_int (prefs_dialog->sensors_applet->settings, TIMEOUT, value);
}

static void prefs_dialog_display_mode_changed(GtkComboBox *display_mode_combo_box,
                                              PrefsDialog *prefs_dialog) {

    int display_mode;

    display_mode = gtk_combo_box_get_active(display_mode_combo_box);

    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->layout_mode_label),
                             (display_mode != DISPLAY_ICON) &&
                             (display_mode != DISPLAY_VALUE) &&
                             (display_mode != DISPLAY_GRAPH));

    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->layout_mode_combo_box),
                             (display_mode != DISPLAY_ICON) &&
                             (display_mode != DISPLAY_VALUE) &&
                             (display_mode != DISPLAY_GRAPH));

    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->graph_size_label),
                             (display_mode == DISPLAY_GRAPH));
    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->graph_size_spinbutton),
                             (display_mode == DISPLAY_GRAPH));

    g_settings_set_int (prefs_dialog->sensors_applet->settings,
                        DISPLAY_MODE,
                        gtk_combo_box_get_active(display_mode_combo_box));

    sensors_applet_display_layout_changed(prefs_dialog->sensors_applet);
}

static void prefs_dialog_layout_mode_changed(GtkComboBox *layout_mode_combo_box,
                                             PrefsDialog *prefs_dialog) {

    g_settings_set_int (prefs_dialog->sensors_applet->settings,
                        LAYOUT_MODE,
                        gtk_combo_box_get_active(layout_mode_combo_box));

    sensors_applet_display_layout_changed(prefs_dialog->sensors_applet);
}

static void prefs_dialog_temperature_scale_changed(GtkComboBox *temperature_scale_combo_box,
                                                   PrefsDialog *prefs_dialog) {

    /* get old temp scale value */
    TemperatureScale scales[2];
    GtkTreeModel *model;

    scales[OLD_TEMP_SCALE] = (TemperatureScale) g_settings_get_int (prefs_dialog->sensors_applet->settings, TEMPERATURE_SCALE);

    scales[NEW_TEMP_SCALE] = (TemperatureScale)gtk_combo_box_get_active(temperature_scale_combo_box);

    g_settings_set_int (prefs_dialog->sensors_applet->settings,
                        TEMPERATURE_SCALE,
                        scales[NEW_TEMP_SCALE]);

    /* now go thru and convert all low and high sensor values in
     * the tree to either celcius or Fahrenheit */
    model = gtk_tree_view_get_model(prefs_dialog->view);
    gtk_tree_model_foreach(model,
                           (GtkTreeModelForeachFunc)prefs_dialog_convert_low_and_high_values,
                           scales);

    /* finally update display of active sensors */
    sensors_applet_update_active_sensors(prefs_dialog->sensors_applet);
}

// hide/show units
static void prefs_dialog_show_units_toggled (GtkCheckButton *show_units, PrefsDialog *prefs_dialog) {
    gboolean state;

    state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_units));
    g_settings_set_boolean (prefs_dialog->sensors_applet->settings, HIDE_UNITS, !state);
    sensors_applet_update_active_sensors (prefs_dialog->sensors_applet);
}

#ifdef HAVE_LIBNOTIFY
static void prefs_dialog_display_notifications_toggled(GtkCheckButton *display_notifications,
                                                       PrefsDialog *prefs_dialog) {

    gboolean notify;

    notify = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(display_notifications));
    g_settings_set_boolean(prefs_dialog->sensors_applet->settings,
                           DISPLAY_NOTIFICATIONS,
                           notify);

    if (notify) {
        sensors_applet_update_active_sensors(prefs_dialog->sensors_applet);
    } else {
        sensors_applet_notify_end_all(prefs_dialog->sensors_applet);
    }
}
#endif

static void prefs_dialog_graph_size_changed(GtkSpinButton *button,
                                            PrefsDialog *prefs_dialog) {

    gint value;
    value = (gint)(gtk_spin_button_get_value(button));
    g_settings_set_int(prefs_dialog->sensors_applet->settings, GRAPH_SIZE, value);

    /* notify change of number of samples */
    sensors_applet_graph_size_changed(prefs_dialog->sensors_applet);

}

/* callbacks for the tree of sensors */
static void prefs_dialog_sensor_toggled(GtkCellRenderer *renderer, gchar *path_str, PrefsDialog *prefs_dialog) {
    GtkTreeIter iter;
    GtkTreePath *path;

    gboolean old_value;

    path = gtk_tree_path_new_from_string(path_str);

    gtk_tree_model_get_iter(GTK_TREE_MODEL(prefs_dialog->sensors_applet->sensors), &iter, path);
    gtk_tree_model_get(GTK_TREE_MODEL(prefs_dialog->sensors_applet->sensors),
                       &iter,
                       ENABLE_COLUMN, &old_value,
                       -1);

    if (old_value) {
        sensors_applet_sensor_disabled(prefs_dialog->sensors_applet, path);
    } else {
        sensors_applet_sensor_enabled(prefs_dialog->sensors_applet, path);
    }

    gtk_tree_store_set(prefs_dialog->sensors_applet->sensors, &iter,
                       ENABLE_COLUMN, !old_value,
                       -1);

    gtk_tree_path_free(path);
}

static void prefs_dialog_sensor_name_changed(GtkCellRenderer *renderer, gchar *path_str, gchar *new_text, PrefsDialog *prefs_dialog) {
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);

    gtk_tree_model_get_iter(GTK_TREE_MODEL(prefs_dialog->sensors_applet->sensors), &iter, path);

    gtk_tree_store_set(prefs_dialog->sensors_applet->sensors, &iter, LABEL_COLUMN, new_text, -1);

    sensors_applet_update_sensor(prefs_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

static void prefs_dialog_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, PrefsDialog *prefs_dialog) {
    /* only bring up dialog this if is a sensor - ie has no
     * children */
    GtkTreeIter iter;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(view);
    /* make sure can set iter first */
    if (gtk_tree_model_get_iter(model, &iter, path) && !gtk_tree_model_iter_has_child(model, &iter)) {
        sensor_config_dialog_create(prefs_dialog->sensors_applet);
    }
}

static void prefs_dialog_sensor_up_button_clicked(GtkButton *button, PrefsDialog *prefs_dialog) {
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(prefs_dialog->sensors_applet->selection, &model, &iter)) {
        /* if has no prev node set up button insentive */
        GtkTreePath *path;

        path = gtk_tree_model_get_path(model, &iter);
        if (gtk_tree_path_prev(path)) {

            GtkTreeIter prev_iter;
            /* check is a valid node in out model */
            if (gtk_tree_model_get_iter(model, &prev_iter, path)) {

                gtk_tree_store_move_before(GTK_TREE_STORE(model),
                                           &iter,
                                           &prev_iter);
                g_signal_emit_by_name(prefs_dialog->sensors_applet->selection, "changed");
                sensors_applet_reorder_sensors(prefs_dialog->sensors_applet);

            }
        }

        gtk_tree_path_free(path);

    }
}

static void prefs_dialog_sensor_down_button_clicked(GtkButton *button, PrefsDialog *prefs_dialog) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeIter iter_next;

    if (gtk_tree_selection_get_selected(prefs_dialog->sensors_applet->selection, &model, &iter)) {
        iter_next = iter;
        /* if has no next node set down button insentive */
        if (gtk_tree_model_iter_next(model, &iter_next)) {

                gtk_tree_store_move_after(GTK_TREE_STORE(model),
                                          &iter,
                                          &iter_next);
                g_signal_emit_by_name(prefs_dialog->sensors_applet->selection, "changed");
                sensors_applet_reorder_sensors(prefs_dialog->sensors_applet);

        }
    }
}

static void prefs_dialog_sensor_config_button_clicked(GtkButton *button, PrefsDialog *prefs_dialog) {
    sensor_config_dialog_create(prefs_dialog->sensors_applet);
}

/* if a sensor is selected, make config sure button is able to be
 * clicked and also set the sensitivities properly for the up and down
 * buttons */
static void prefs_dialog_selection_changed(GtkTreeSelection *selection,
                                           PrefsDialog *prefs_dialog) {

    GtkTreeIter iter;
    GtkTreeModel *model;

    /* if there is a selection with no children make config button sensitive */
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        GtkTreePath *path;

        if (!gtk_tree_model_iter_has_child(model, &iter)) {
            gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_config_button), TRUE);
        } else {
            /* otherwise make insensitive */
            gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_config_button), FALSE);
        }

        /* if has no prev node set up button insentive */
        path = gtk_tree_model_get_path(model, &iter);
        if (gtk_tree_path_prev(path)) {
            GtkTreeIter prev_iter;
            /* check is a valid node in out model */
            if (gtk_tree_model_get_iter(model, &prev_iter, path)) {
                gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_up_button), TRUE);
            } else {
                gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_up_button), FALSE);
            }
        }  else {
            gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_up_button), FALSE);
        }

        gtk_tree_path_free(path);

        /* if has no next node set down button insentive */
        if (gtk_tree_model_iter_next(model, &iter)) {
            gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_down_button), TRUE);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_down_button), FALSE);
        }

    } else {
        /* otherwise make all insensitive */
        gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_config_button), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_up_button), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_down_button), FALSE);

    }
}

void prefs_dialog_open(SensorsApplet *sensors_applet) {
    gchar *header_text;
    PrefsDialog *prefs_dialog;
    DisplayMode display_mode;
    GtkWidget *content_area;

    g_assert(sensors_applet->prefs_dialog == NULL);

    /* while prefs dialog is open, stop the updating of sensors so
     * we don't get any race conditions due to concurrent updates
     * of the labels, values and icons linked lists etc. */
    if (sensors_applet->timeout_id != 0) {
        if (g_source_remove(sensors_applet->timeout_id)) {
            sensors_applet->timeout_id = 0;
        }
    }

    sensors_applet->prefs_dialog = g_new0(PrefsDialog, 1);
    prefs_dialog = sensors_applet->prefs_dialog;

    prefs_dialog->sensors_applet = sensors_applet;

    prefs_dialog->dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(_("Sensors Applet Preferences"),
                                                                  NULL,
                                                                  0,
                                                                  "gtk-help",
                                                                  GTK_RESPONSE_HELP,
                                                                  "gtk-close",
                                                                  GTK_RESPONSE_CLOSE,
                                                                  NULL));

    gtk_window_set_icon_name(GTK_WINDOW(prefs_dialog->dialog), "mate-sensors-applet");

    g_object_set(prefs_dialog->dialog,
                 "border-width", 12,
                 "default-width", 480,
                 "default-height", 350,
                 NULL);

    content_area = gtk_dialog_get_content_area (prefs_dialog->dialog);
    gtk_box_set_homogeneous(GTK_BOX(content_area), FALSE);

    gtk_box_set_spacing(GTK_BOX(content_area), 5);

    g_signal_connect(prefs_dialog->dialog,
                     "response", G_CALLBACK(prefs_dialog_response),
                     sensors_applet);

    g_signal_connect_swapped(prefs_dialog->dialog,
                             "delete-event", G_CALLBACK(prefs_dialog_close),
                             sensors_applet);

    g_signal_connect_swapped(prefs_dialog->dialog,
                             "destroy", G_CALLBACK(prefs_dialog_close),
                             sensors_applet);

    /* if no SensorsList's have been created, this is because
       we haven't been able to access any sensors */
    if (sensors_applet->sensors == NULL) {
        GtkWidget *label = gtk_label_new(_("No sensors found!"));
        gtk_box_pack_start (GTK_BOX(content_area), label, FALSE, FALSE, 0);
        return;
    }

    header_text = g_markup_printf_escaped("<b>%s</b>", _("Display"));
    prefs_dialog->display_header = g_object_new(GTK_TYPE_LABEL,
                                                "use-markup", TRUE,
                                                "label", header_text,
                                                "xalign", 0.0,
                                                NULL);
    g_free(header_text);

    prefs_dialog->display_mode_combo_box = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    /*expand the whole column */
    gtk_widget_set_hexpand(GTK_WIDGET(prefs_dialog->display_mode_combo_box), TRUE);

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("label with value"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("icon with value"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("value only"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("icon only"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prefs_dialog->display_mode_combo_box), _("graph only"));

    display_mode = g_settings_get_int(sensors_applet->settings, DISPLAY_MODE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(prefs_dialog->display_mode_combo_box), display_mode);

    g_signal_connect(prefs_dialog->display_mode_combo_box,
                     "changed",
                     G_CALLBACK(prefs_dialog_display_mode_changed),
                     prefs_dialog);

    /* use spaces in label to indent */
    prefs_dialog->display_mode_label = g_object_new(GTK_TYPE_LABEL,
                                                    "use-underline", TRUE,
                                                    "label", _("_Display sensors in panel as"),
                                                    "mnemonic-widget", prefs_dialog->display_mode_combo_box,
                                                    "xalign", 0.0,
                                                    NULL);

    prefs_dialog->layout_mode_combo_box = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());

    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->layout_mode_combo_box),
                             (display_mode != DISPLAY_ICON) &&
                             (display_mode != DISPLAY_VALUE) &&
                             (display_mode != DISPLAY_GRAPH));

    gtk_combo_box_text_append_text(prefs_dialog->layout_mode_combo_box, _("beside labels / icons"));
    gtk_combo_box_text_append_text(prefs_dialog->layout_mode_combo_box, _("below labels / icons"));

    gtk_combo_box_set_active(
            GTK_COMBO_BOX(prefs_dialog->layout_mode_combo_box),
            g_settings_get_int(sensors_applet->settings, LAYOUT_MODE));

    g_signal_connect(prefs_dialog->layout_mode_combo_box,
                     "changed",
                     G_CALLBACK(prefs_dialog_layout_mode_changed),
                     prefs_dialog);

    prefs_dialog->layout_mode_label = g_object_new(GTK_TYPE_LABEL,
                                                   "use-underline", TRUE,
                                                   "label", _("Preferred _position of sensor values"),
                                                   "mnemonic-widget", prefs_dialog->layout_mode_combo_box,
                                                   "xalign", 0.0,
                                                   NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->layout_mode_label),
                             (display_mode != DISPLAY_ICON) &&
                             (display_mode != DISPLAY_VALUE) &&
                             (display_mode != DISPLAY_GRAPH));

    prefs_dialog->temperature_scale_combo_box = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());

    gtk_combo_box_text_append_text(prefs_dialog->temperature_scale_combo_box, _("Kelvin"));
    gtk_combo_box_text_append_text(prefs_dialog->temperature_scale_combo_box, _("Celsius"));
    gtk_combo_box_text_append_text(prefs_dialog->temperature_scale_combo_box, _("Fahrenheit"));

    gtk_combo_box_set_active(
            GTK_COMBO_BOX(prefs_dialog->temperature_scale_combo_box),
            g_settings_get_int(sensors_applet->settings, TEMPERATURE_SCALE));

    g_signal_connect(prefs_dialog->temperature_scale_combo_box,
                     "changed",
                     G_CALLBACK(prefs_dialog_temperature_scale_changed),
                     prefs_dialog);

    prefs_dialog->temperature_scale_label = g_object_new(GTK_TYPE_LABEL,
                                                         "use-underline", TRUE,
                                                         "label", _("_Temperature scale"),
                                                         "mnemonic-widget", prefs_dialog->temperature_scale_combo_box,
                                                         "xalign", 0.0,
                                                         NULL);

    prefs_dialog->graph_size_adjust = g_object_new(GTK_TYPE_ADJUSTMENT,
                                                   "value", (gdouble)g_settings_get_int(sensors_applet->settings, GRAPH_SIZE),
                                                   "lower", 1.0,
                                                   "upper", 100.0,
                                                   "step-increment", 1.0,
                                                   "page-increment", 10.0,
                                                   "page-size", 0.0,
                                                   NULL);

    prefs_dialog->graph_size_spinbutton = g_object_new(GTK_TYPE_SPIN_BUTTON,
                                                       "adjustment", prefs_dialog->graph_size_adjust,
                                                       "climb-rate", 1.0,
                                                       "digits", 0,
                                                       "value", (gdouble)g_settings_get_int(sensors_applet->settings, GRAPH_SIZE),
                                                       "width-chars", 4,
                                                       NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->graph_size_spinbutton), (display_mode == DISPLAY_GRAPH));

    prefs_dialog->graph_size_label = g_object_new(GTK_TYPE_LABEL,
                                                  "use-underline", TRUE,
                                                  "label", _("Graph _size (pixels)"),
                                                  "mnemonic-widget", prefs_dialog->graph_size_spinbutton,
                                                  "xalign", 0.0,
                                                  NULL);

    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->graph_size_label), (display_mode == DISPLAY_GRAPH));

    g_signal_connect(prefs_dialog->graph_size_spinbutton, "value-changed",
                     G_CALLBACK(prefs_dialog_graph_size_changed),
                     prefs_dialog);

    // hide/show units
    prefs_dialog->show_units = gtk_check_button_new_with_label (_("Show _units"));
    gtk_button_set_use_underline (GTK_BUTTON (prefs_dialog->show_units), TRUE);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs_dialog->show_units),
                        !g_settings_get_boolean (sensors_applet->settings, HIDE_UNITS));
    g_signal_connect (prefs_dialog->show_units, "toggled",
                        G_CALLBACK (prefs_dialog_show_units_toggled),
                        prefs_dialog);

    header_text = g_markup_printf_escaped("<b>%s</b>", _("Update"));
    prefs_dialog->update_header = g_object_new(GTK_TYPE_LABEL,
                                               "use-markup", TRUE,
                                               "label", header_text,
                                               "xalign", 0.0,
                                               NULL);
    g_free(header_text);

    prefs_dialog->timeout_adjust = g_object_new(GTK_TYPE_ADJUSTMENT,
                                                "value", 2.0,
                                                "lower", 1.5,
                                                "upper", 10.0,
                                                "step-increment", 0.5,
                                                "page-increment", 1.0,
                                                "page-size", 0.0,
                                                NULL);

    prefs_dialog->timeout_spinbutton = g_object_new(GTK_TYPE_SPIN_BUTTON,
                                                    "adjustment", prefs_dialog->timeout_adjust,
                                                    "climb-rate", 0.5,
                                                    "digits", 1,
                                                    "value", (gdouble) g_settings_get_int(sensors_applet->settings, TIMEOUT) / 1000.0,
                                                    "width-chars", 4,
                                                    NULL);

    prefs_dialog->timeout_label = g_object_new(GTK_TYPE_LABEL,
                                               "use-underline", TRUE,
                                               "label", _("Update _interval (secs)"),
                                               "mnemonic-widget", prefs_dialog->timeout_spinbutton,
                                               "xalign", 0.0,
                                               NULL);

    g_signal_connect(prefs_dialog->timeout_spinbutton, "value-changed",
                     G_CALLBACK(prefs_dialog_timeout_changed),
                     prefs_dialog);

#ifdef HAVE_LIBNOTIFY
    header_text = g_markup_printf_escaped("<b>%s</b>", _("Notifications"));
    prefs_dialog->notifications_header = g_object_new(GTK_TYPE_LABEL,
                                                      "use-markup", TRUE,
                                                      "label", header_text,
                                                      "xalign", 0.0,
                                                      NULL);
    g_free(header_text);

    prefs_dialog->display_notifications = g_object_new(GTK_TYPE_CHECK_BUTTON,
                                                       "use-underline", TRUE,
                                                       "label", _("Display _notifications"),
                                                       NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prefs_dialog->display_notifications),
                                 g_settings_get_boolean(sensors_applet->settings,
                                                        DISPLAY_NOTIFICATIONS));
    g_signal_connect(prefs_dialog->display_notifications,
                     "toggled",
                     G_CALLBACK(prefs_dialog_display_notifications_toggled),
                     prefs_dialog);
#endif

    /* SIZE AND LAYOUT */
    /* keep all widgets same size */
    prefs_dialog->size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

    gtk_size_group_add_widget(prefs_dialog->size_group,
                              GTK_WIDGET(prefs_dialog->display_mode_combo_box));

    gtk_size_group_add_widget(prefs_dialog->size_group,
                              GTK_WIDGET(prefs_dialog->layout_mode_combo_box));

    gtk_size_group_add_widget(prefs_dialog->size_group,
                              GTK_WIDGET(prefs_dialog->temperature_scale_combo_box));

    gtk_size_group_add_widget(prefs_dialog->size_group,
                              GTK_WIDGET(prefs_dialog->timeout_spinbutton));

    g_object_unref(prefs_dialog->size_group);

    prefs_dialog->globals_grid = g_object_new(GTK_TYPE_GRID,
                                               "row-homogeneous", FALSE,
                                               "column-homogeneous", FALSE,
                                               "row-spacing", 6,
                                               "column-spacing", 12,
                                               NULL);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->display_header),
                     0, 0, 2, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->display_mode_label),
                     1, 1, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->display_mode_combo_box),
                     2, 1, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->layout_mode_label),
                     1, 2, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->layout_mode_combo_box),
                     2, 2, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->graph_size_label),
                     1, 3, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->graph_size_spinbutton),
                     2, 3, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->temperature_scale_label),
                     1, 4, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->temperature_scale_combo_box),
                     2, 4, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->show_units),
                     1, 5, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->update_header),
                     0, 6, 2, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->timeout_label),
                     1, 7, 1, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->timeout_spinbutton),
                     2, 7, 1, 1);

#ifdef HAVE_LIBNOTIFY
    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->notifications_header),
                     0, 8, 2, 1);

    gtk_grid_attach(prefs_dialog->globals_grid,
                     GTK_WIDGET(prefs_dialog->display_notifications),
                     1, 9, 1, 1);
#endif

    gtk_widget_set_valign(GTK_WIDGET(prefs_dialog->globals_grid), GTK_ALIGN_START);
    gtk_widget_set_margin_start(GTK_WIDGET(prefs_dialog->globals_grid), 12);
    gtk_widget_set_margin_end(GTK_WIDGET(prefs_dialog->globals_grid), 12);
    gtk_widget_set_margin_top(GTK_WIDGET(prefs_dialog->globals_grid), 12);
    gtk_widget_set_margin_bottom(GTK_WIDGET(prefs_dialog->globals_grid), 12);

    prefs_dialog->view = g_object_new(GTK_TYPE_TREE_VIEW,
                                      "model", GTK_TREE_MODEL(sensors_applet->sensors),
                                      "rules-hint", TRUE,
                                      "reorderable", FALSE,
                                      "enable-search", TRUE,
                                      "search-column", LABEL_COLUMN,
                                      NULL);

    /* get double clicks on rows - do same as configure sensor
     * button clicks */
    g_signal_connect(prefs_dialog->view, "row-activated",
                     G_CALLBACK(prefs_dialog_row_activated),
                     prefs_dialog);

    prefs_dialog->id_renderer = gtk_cell_renderer_text_new();
    prefs_dialog->label_renderer = gtk_cell_renderer_text_new();
    g_object_set(prefs_dialog->label_renderer,
                 "editable", TRUE,
                 NULL);

    g_signal_connect(prefs_dialog->label_renderer, "edited",
                     G_CALLBACK(prefs_dialog_sensor_name_changed),
                     prefs_dialog);

    prefs_dialog->enable_renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(prefs_dialog->enable_renderer, "toggled",
                     G_CALLBACK(prefs_dialog_sensor_toggled),
                     prefs_dialog);
    prefs_dialog->icon_renderer = gtk_cell_renderer_pixbuf_new();

    prefs_dialog->id_column = gtk_tree_view_column_new_with_attributes(_("Sensor"),
                                                                       prefs_dialog->id_renderer,
                                                                       "text", ID_COLUMN,
                                                                       NULL);

    gtk_tree_view_column_set_min_width(prefs_dialog->id_column, 90);

    prefs_dialog->label_column = gtk_tree_view_column_new_with_attributes(_("Label"),
                                                                          prefs_dialog->label_renderer,
                                                                          "text", LABEL_COLUMN,
                                                                          "visible", VISIBLE_COLUMN,
                                                                          NULL);

    gtk_tree_view_column_set_min_width(prefs_dialog->label_column, 100);

    /* create the tooltip */
    gtk_widget_set_tooltip_text(GTK_WIDGET(prefs_dialog->view),
                                _("Labels can be edited directly by clicking on them."));
    prefs_dialog->enable_column = gtk_tree_view_column_new_with_attributes(_("Enabled"),
                                                                           prefs_dialog->enable_renderer,
                                                                           "active", ENABLE_COLUMN,
                                                                           "visible", VISIBLE_COLUMN,
                                                                           NULL);

    gtk_tree_view_column_set_min_width(prefs_dialog->enable_column, 90);

    prefs_dialog->icon_column = gtk_tree_view_column_new_with_attributes(_("Icon"),
                                                                         prefs_dialog->icon_renderer,
                                                                         "pixbuf", ICON_PIXBUF_COLUMN,
                                                                         "visible", VISIBLE_COLUMN,
                                                                         NULL);

    gtk_tree_view_append_column(prefs_dialog->view, prefs_dialog->id_column);
    gtk_tree_view_append_column(prefs_dialog->view, prefs_dialog->icon_column);
    gtk_tree_view_append_column(prefs_dialog->view, prefs_dialog->label_column);
    gtk_tree_view_append_column(prefs_dialog->view, prefs_dialog->enable_column);

    gtk_tree_view_columns_autosize(prefs_dialog->view);

    prefs_dialog->scrolled_window = g_object_new(GTK_TYPE_SCROLLED_WINDOW,
                                                 "hadjustment", NULL,
                                                 "height-request", 200,
                                                 "hscrollbar-policy", GTK_POLICY_NEVER,
                                                 "vadjustment",NULL,
                                                 "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                                                 NULL);

    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(prefs_dialog->scrolled_window), GTK_SHADOW_IN);

    gtk_container_add(GTK_CONTAINER(prefs_dialog->scrolled_window), GTK_WIDGET(prefs_dialog->view));

    /* GtkTree Selection */
    sensors_applet->selection = gtk_tree_view_get_selection(prefs_dialog->view);
    /* allow user to only select one row at a time at most */
    gtk_tree_selection_set_mode(sensors_applet->selection, GTK_SELECTION_SINGLE);
    /* when selection is changed, make sure sensor_config button is activated */

    /* Create buttons for user to interact with sensors tree */
    prefs_dialog->sensor_up_button = GTK_BUTTON(gtk_button_new_with_mnemonic(_("_Up")));
    gtk_button_set_image(prefs_dialog->sensor_up_button, gtk_image_new_from_icon_name("go-up", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_up_button), FALSE);

    g_signal_connect(prefs_dialog->sensor_up_button, "clicked",
                     G_CALLBACK(prefs_dialog_sensor_up_button_clicked),
                     prefs_dialog);

    prefs_dialog->sensor_down_button = GTK_BUTTON(gtk_button_new_with_mnemonic(_("_Down")));
    gtk_button_set_image(prefs_dialog->sensor_down_button, gtk_image_new_from_icon_name("go-down", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_sensitive(GTK_WIDGET(prefs_dialog->sensor_down_button), FALSE);

    g_signal_connect(prefs_dialog->sensor_down_button, "clicked",
                     G_CALLBACK(prefs_dialog_sensor_down_button_clicked),
                     prefs_dialog);

    prefs_dialog->buttons_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_VERTICAL));

    gtk_button_box_set_layout(GTK_BUTTON_BOX(prefs_dialog->buttons_box), GTK_BUTTONBOX_SPREAD);

    gtk_box_pack_start(GTK_BOX(prefs_dialog->buttons_box), GTK_WIDGET(prefs_dialog->sensor_up_button), FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(prefs_dialog->buttons_box), GTK_WIDGET(prefs_dialog->sensor_down_button), FALSE, FALSE, 0);

    prefs_dialog->sensors_hbox = g_object_new(GTK_TYPE_BOX,
                                              "orientation", GTK_ORIENTATION_HORIZONTAL,
                                              "border-width", 5,
                                              "homogeneous", FALSE,
                                              "spacing", 5,
                                              NULL);

    gtk_box_pack_start(prefs_dialog->sensors_hbox,
                       GTK_WIDGET(prefs_dialog->scrolled_window),
                       TRUE, TRUE, 0); /* make sure window takes up most of room */

    gtk_box_pack_start(prefs_dialog->sensors_hbox,
                       GTK_WIDGET(prefs_dialog->buttons_box),
                       FALSE, FALSE, 0);

    /* Sensor Config button */
    /* initially make button insensitive until user selects a row
       from the sensors tree */
    prefs_dialog->sensor_config_button = GTK_BUTTON(gtk_button_new_with_mnemonic(_("_Properties")));
    gtk_button_set_image(prefs_dialog->sensor_config_button, gtk_image_new_from_icon_name("document-properties", GTK_ICON_SIZE_BUTTON));
    g_object_set(prefs_dialog->sensor_config_button,
                 "sensitive", FALSE,
                 NULL);

    g_signal_connect(sensors_applet->selection,
                     "changed",
                     G_CALLBACK(prefs_dialog_selection_changed),
                     prefs_dialog);

    /* pass selection to signal handler so we can give user a
       sensors_applet->prefs_dialog with the selected rows alarm
       value and enable */
    g_signal_connect(prefs_dialog->sensor_config_button, "clicked",
                     G_CALLBACK(prefs_dialog_sensor_config_button_clicked),
                     prefs_dialog);

    prefs_dialog->sensor_config_hbox = g_object_new(GTK_TYPE_BOX,
                                                    "orientation", GTK_ORIENTATION_HORIZONTAL,
                                                    "border-width", 5,
                                                    "homogeneous", FALSE,
                                                    "spacing", 0,
                                                    NULL);
    gtk_box_pack_end(prefs_dialog->sensor_config_hbox,
                     GTK_WIDGET(prefs_dialog->sensor_config_button),
                     FALSE, FALSE, 0);

    /* pack sensors_vbox */
    prefs_dialog->sensors_vbox = g_object_new(GTK_TYPE_BOX,
                                              "orientation", GTK_ORIENTATION_VERTICAL,
                                              "border-width", 5,
                                              "homogeneous", FALSE,
                                              "spacing", 0,
                                              NULL);

    gtk_box_pack_start(prefs_dialog->sensors_vbox,
                       GTK_WIDGET(prefs_dialog->sensors_hbox),
                       TRUE, TRUE, 0);
    gtk_box_pack_start(prefs_dialog->sensors_vbox,
                       GTK_WIDGET(prefs_dialog->sensor_config_hbox),
                       FALSE, FALSE, 0);

    prefs_dialog->notebook = g_object_new(GTK_TYPE_NOTEBOOK, NULL);

    gtk_notebook_append_page(prefs_dialog->notebook,
                             GTK_WIDGET(prefs_dialog->globals_grid),
                             gtk_label_new(_("General Options")));

    gtk_notebook_append_page(prefs_dialog->notebook,
                             GTK_WIDGET(prefs_dialog->sensors_vbox),
                             gtk_label_new(_("Sensors")));

    /* pack notebook into prefs_dialog */
    content_area = gtk_dialog_get_content_area (prefs_dialog->dialog);
    gtk_box_pack_start (GTK_BOX(content_area), GTK_WIDGET(prefs_dialog->notebook), TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(prefs_dialog->dialog));
}
