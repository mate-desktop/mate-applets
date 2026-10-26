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
#include "sensor-config-dialog.h"
#include "sensors-applet.h"

#define SPINBUTTON_WIDTH_CHARS 8
#define VALUE_DECIMAL_PLACES 3

typedef struct {
    SensorsApplet *sensors_applet;

    GtkDialog *dialog;
    /* icon widgets */
    GtkLabel *icon_header, *icon_type_label;
    GtkComboBox *icon_type_combo_box;
    GtkAlignment *icon_type_combo_box_aligner;
    GtkCellRenderer *icon_renderer;

    /* Graph Color chooser */
    GtkColorButton *graph_color_button;
    GtkAlignment *graph_color_button_aligner;
    GtkLabel *graph_color_label, *graph_header;

    /* multiplier and offset widgets */
    GtkLabel *scale_header, *multiplier_label, *offset_label;
    GtkAlignment *multiplier_spinbutton_aligner, *offset_spinbutton_aligner;
    GtkAdjustment *multiplier_adjust, *offset_adjust;
    GtkSpinButton *multiplier_spinbutton, *offset_spinbutton;

    GtkLabel *limits_header;
    GtkLabel *low_value_label, *high_value_label;
    GtkAlignment *low_value_spinbutton_aligner, *high_value_spinbutton_aligner;
    GtkAdjustment *low_value_adjust, *high_value_adjust;
    GtkSpinButton *low_value_spinbutton, *high_value_spinbutton;

    /* alarm widgets */
    GtkLabel *alarm_header;
    GtkLabel *low_alarm_command_label, *high_alarm_command_label, *alarm_timeout_label;
    GtkAlignment *alarm_timeout_spinbutton_aligner;
    GtkAdjustment *alarm_timeout_adjust;
    GtkSpinButton *alarm_timeout_spinbutton;
    GtkGrid *grid;
    GtkAlignment *alarm_enable_aligner;
    GtkCheckButton *alarm_enable_checkbutton;
    GtkEntry *low_alarm_command_entry, *high_alarm_command_entry;

    GtkSizeGroup *size_group;
} SensorConfigDialog;

static void sensor_config_dialog_response(GtkDialog *dialog,
                                          gint response,
                                          gpointer data) {

    GError *error = NULL;

    switch (response) {
        case GTK_RESPONSE_HELP:
            g_debug("loading help in config dialog");
            gtk_show_uri_on_window(NULL,
                         "help:mate-sensors-applet/sensors-applet-sensors#sensors-applet-sensor-config-dialog",
                         gtk_get_current_event_time(),
                         &error);

            if (error) {
                    g_debug("Could not open help document: %s ",error->message);
                    g_error_free (error);
            }
            break;

        default:
            g_debug("destroying config dialog");
            gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}

static void sensor_config_dialog_multiplier_changed(GtkSpinButton *spinbutton, SensorConfigDialog *config_dialog) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gdouble value;

    value = gtk_spin_button_get_value(spinbutton);

    gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                    &model,
                                    &iter);

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(config_dialog->sensors_applet->sensors), &iter);
    gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                        &iter,
                        MULTIPLIER_COLUMN, value,
                       -1);

    sensors_applet_update_sensor(config_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

static void sensor_config_dialog_offset_changed(GtkSpinButton *spinbutton, SensorConfigDialog *config_dialog) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gdouble value;

    value = gtk_spin_button_get_value(spinbutton);

    gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                    &model,
                                    &iter);
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(config_dialog->sensors_applet->sensors), &iter);
    gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                        &iter,
                        OFFSET_COLUMN, value,
                       -1);

    sensors_applet_update_sensor(config_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

static void sensor_config_dialog_low_value_changed(GtkSpinButton *spinbutton, SensorConfigDialog *config_dialog) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gdouble value;

    value = gtk_spin_button_get_value(spinbutton);

    gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                    &model,
                                    &iter);
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(config_dialog->sensors_applet->sensors), &iter);

    gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                        &iter,
                        LOW_VALUE_COLUMN, value,
                       -1);

    sensors_applet_update_sensor(config_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

static void sensor_config_dialog_high_value_changed(GtkSpinButton *spinbutton, SensorConfigDialog *config_dialog) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gdouble value;

    value = gtk_spin_button_get_value(spinbutton);

    gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                    &model,
                                    &iter);
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(config_dialog->sensors_applet->sensors), &iter);

    gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                        &iter,
                        HIGH_VALUE_COLUMN, value,
                       -1);

    sensors_applet_update_sensor(config_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

static void sensor_config_dialog_alarm_toggled(GtkToggleButton *button, SensorConfigDialog *config_dialog) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;

    gboolean value;
    value = gtk_toggle_button_get_active(button);

    /* update state of alarm widgets */
    gtk_widget_set_sensitive(GTK_WIDGET(config_dialog->alarm_timeout_label), value);
    gtk_widget_set_sensitive(GTK_WIDGET(config_dialog->alarm_timeout_spinbutton), value);
    gtk_widget_set_sensitive(GTK_WIDGET(config_dialog->low_alarm_command_label), value);
    gtk_widget_set_sensitive(GTK_WIDGET(config_dialog->low_alarm_command_entry), value);
    gtk_widget_set_sensitive(GTK_WIDGET(config_dialog->high_alarm_command_label), value);
    gtk_widget_set_sensitive(GTK_WIDGET(config_dialog->high_alarm_command_entry), value);

    gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                    &model,
                                    &iter);

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(config_dialog->sensors_applet->sensors), &iter);

    gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                        &iter,
                        ALARM_ENABLE_COLUMN, value,
                       -1);

    sensors_applet_update_sensor(config_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

static void sensor_config_dialog_alarm_timeout_changed(GtkSpinButton *spinbutton, SensorConfigDialog *config_dialog) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gint value;

    value = gtk_spin_button_get_value_as_int(spinbutton);

    gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                    &model,
                                    &iter);
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(config_dialog->sensors_applet->sensors), &iter);

    sensors_applet_all_alarms_off(config_dialog->sensors_applet, path);
    gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                        &iter,
                        ALARM_TIMEOUT_COLUMN, value,
                       -1);

    sensors_applet_update_sensor(config_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

static void sensor_config_dialog_alarm_command_edited(GtkEntry *command_entry, SensorConfigDialog *config_dialog, NotifType notif_type) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;

    gchar *value;
    g_object_get(command_entry, "text", &value, NULL);

    gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                    &model,
                                    &iter);
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(config_dialog->sensors_applet->sensors), &iter);

    sensors_applet_alarm_off(config_dialog->sensors_applet, path, notif_type);

    gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                       &iter,
                       (notif_type == LOW_ALARM ? LOW_ALARM_COMMAND_COLUMN : HIGH_ALARM_COMMAND_COLUMN),
                       value,
                       -1);
    g_free(value);
    sensors_applet_update_sensor(config_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

static void sensor_config_dialog_low_alarm_command_edited(GtkEntry *command_entry, SensorConfigDialog *config_dialog) {
    sensor_config_dialog_alarm_command_edited(command_entry,
                                              config_dialog,
                                              LOW_ALARM);
}

static void sensor_config_dialog_high_alarm_command_edited(GtkEntry *command_entry, SensorConfigDialog *config_dialog) {
    sensor_config_dialog_alarm_command_edited(command_entry,
                                              config_dialog,
                                              HIGH_ALARM);
}

static void sensor_config_dialog_icon_type_changed(GtkComboBox *icon_type_combo_box,
                                                   SensorConfigDialog *config_dialog) {
    GtkTreeModel *icons_model;
    GtkTreeIter icons_iter;

    GtkTreeModel *model;
    GtkTreeIter iter;

    GdkPixbuf *new_icon;
    IconType icon_type;

    icons_model = gtk_combo_box_get_model(icon_type_combo_box);
    if (gtk_combo_box_get_active_iter(icon_type_combo_box, &icons_iter)) {
        GtkTreePath *path;

        gtk_tree_model_get(icons_model, &icons_iter,
                           0, &new_icon,
                           -1);

        icon_type = gtk_combo_box_get_active(icon_type_combo_box);
        gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                        &model,
                                        &iter);

        path = gtk_tree_model_get_path(model, &iter);
        gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                           &iter,
                           ICON_TYPE_COLUMN, icon_type,
                           ICON_PIXBUF_COLUMN, new_icon,
                           -1);
        g_object_unref(new_icon);
        sensors_applet_icon_changed(config_dialog->sensors_applet, path);
        gtk_tree_path_free(path);
    }
}

static void sensor_config_dialog_graph_color_set(GtkColorChooser *color_chooser,
                                                 SensorConfigDialog *config_dialog) {
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *color_string;
    GdkRGBA color;

    gtk_color_chooser_get_rgba(color_chooser, &color);

    color_string = g_strdup_printf ("#%02X%02X%02X", (int)(0.5 + CLAMP (color.red, 0., 1.) * 255.),
                                    (int)(0.5 + CLAMP (color.green, 0., 1.) * 255.),
                                    (int)(0.5 + CLAMP (color.blue, 0., 1.) * 255.));

    gtk_tree_selection_get_selected(config_dialog->sensors_applet->selection,
                                    &model,
                                    &iter);

    gtk_tree_store_set(config_dialog->sensors_applet->sensors,
                       &iter,
                       GRAPH_COLOR_COLUMN, color_string,
                       -1);

    g_free(color_string);

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(config_dialog->sensors_applet->sensors), &iter);
    sensors_applet_update_sensor(config_dialog->sensors_applet, path);
    gtk_tree_path_free(path);
}

void sensor_config_dialog_create(SensorsApplet *sensors_applet) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkWidget *content_area;

    SensorConfigDialog *config_dialog;

    GtkListStore *icon_store;
    IconType count;
    GdkRGBA graph_color;
    gchar *sensor_label;
    gchar *header_text;

    /* instance variables for data */
    gdouble low_value, high_value, multiplier, offset;
    gboolean alarm_enable;
    gchar *low_alarm_command, *high_alarm_command;
    gint alarm_timeout;
    IconType icon_type;
    gchar *graph_color_string;

    config_dialog = g_new0(SensorConfigDialog, 1);
    config_dialog->sensors_applet = sensors_applet;

    gtk_tree_selection_get_selected(sensors_applet->selection,
                                    &model,
                                    &iter);
    /* get current values of alarm and its enable */
    gtk_tree_model_get(model, &iter,
                       LOW_VALUE_COLUMN, &low_value,
                       HIGH_VALUE_COLUMN, &high_value,
                       ALARM_ENABLE_COLUMN, &alarm_enable,
                       LOW_ALARM_COMMAND_COLUMN, &low_alarm_command,
                       HIGH_ALARM_COMMAND_COLUMN, &high_alarm_command,
                       ALARM_TIMEOUT_COLUMN, &alarm_timeout,
                       MULTIPLIER_COLUMN, &multiplier,
                       OFFSET_COLUMN, &offset,
                       ICON_TYPE_COLUMN, &icon_type,
                       GRAPH_COLOR_COLUMN, &graph_color_string,
                       LABEL_COLUMN, &sensor_label,
                       -1);
    header_text = g_strdup_printf("%s - %s", _("Sensor Properties"), sensor_label);

    config_dialog->dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(header_text,
                                                        GTK_WINDOW(sensors_applet->prefs_dialog->dialog),
                                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                                        "gtk-help",
                                                        GTK_RESPONSE_HELP,
                                                        "gtk-close",
                                                        GTK_RESPONSE_CLOSE,
                                                        NULL));

    gtk_window_set_icon_name(GTK_WINDOW(config_dialog->dialog), "sensors-applet");

    g_free(header_text);
    g_free(sensor_label);

    g_object_set(config_dialog->dialog,
                 "border-width", 12,
                 NULL);

    g_signal_connect(config_dialog->dialog,
                     "response",
                     G_CALLBACK(sensor_config_dialog_response),
                     config_dialog);

    /* graph stuff */
    header_text = g_markup_printf_escaped("<b>%s</b>", _("Graph"));
    config_dialog->graph_header = g_object_new(GTK_TYPE_LABEL,
                               "use-markup", TRUE,
                               "label", header_text,
                               "xalign", 0.0,
                               NULL);
    g_free(header_text);

    gdk_rgba_parse(&graph_color, graph_color_string);
    config_dialog->graph_color_button = GTK_COLOR_BUTTON(gtk_color_button_new_with_rgba(&graph_color));
    gtk_widget_set_halign (GTK_WIDGET(config_dialog->graph_color_button), GTK_ALIGN_START);
    gtk_widget_set_valign (GTK_WIDGET(config_dialog->graph_color_button), GTK_ALIGN_START);

    gtk_color_button_set_title(config_dialog->graph_color_button, _("Graph Color"));

    config_dialog->graph_color_label = g_object_new(GTK_TYPE_LABEL,
                                     "label", _("Graph _color"),
                                    "mnemonic-widget", config_dialog->graph_color_button,
                                    "use-underline", TRUE,
                                    "xalign", 0.0,
                                    NULL);

    g_signal_connect(config_dialog->graph_color_button, "color-set",
                     G_CALLBACK(sensor_config_dialog_graph_color_set),
                     config_dialog);

    /* icon stuff */
    header_text = g_markup_printf_escaped("<b>%s</b>", _("Icon"));
    config_dialog->icon_header = g_object_new(GTK_TYPE_LABEL,
                               "use-markup", TRUE,
                               "label", header_text,
                               "xalign", 0.0,
                               NULL);
    g_free(header_text);

    /* icon type */
    icon_store = gtk_list_store_new(1, GDK_TYPE_PIXBUF);

    /* populate list with icons */
    for (count = CPU_ICON; count < NUM_ICONS; count++) {
        GdkPixbuf *pixbuf;

        pixbuf = sensors_applet_load_icon(count);
        if (pixbuf) {
            gtk_list_store_insert(icon_store, &iter, count);
            gtk_list_store_set(icon_store, &iter, 0, pixbuf, -1);
            /* let list hold icons */
            g_object_unref(pixbuf);
        }
    }

    config_dialog->icon_type_combo_box = GTK_COMBO_BOX(gtk_combo_box_new_with_model(GTK_TREE_MODEL(icon_store)));

        gtk_widget_set_halign (GTK_WIDGET(config_dialog->icon_type_combo_box), GTK_ALIGN_START);
        gtk_widget_set_valign (GTK_WIDGET(config_dialog->icon_type_combo_box), GTK_ALIGN_START);

    config_dialog->icon_renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(config_dialog->icon_type_combo_box),
                               GTK_CELL_RENDERER(config_dialog->icon_renderer),
                               FALSE);

    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(config_dialog->icon_type_combo_box),
                                  GTK_CELL_RENDERER(config_dialog->icon_renderer),
                                  "pixbuf", 0);

    gtk_combo_box_set_active(config_dialog->icon_type_combo_box, icon_type);

    g_signal_connect(config_dialog->icon_type_combo_box, "changed",
                     G_CALLBACK(sensor_config_dialog_icon_type_changed),
                     config_dialog);

    config_dialog->icon_type_label = g_object_new(GTK_TYPE_LABEL,
                                     "label", _("Sensor _icon"),
                                    "mnemonic-widget", config_dialog->icon_type_combo_box,
                                    "use-underline", TRUE,
                                    "xalign", 0.0,
                                    NULL);

    header_text = g_markup_printf_escaped("<b>%s</b>", _("Scaling Parameters"));
    config_dialog->scale_header = g_object_new(GTK_TYPE_LABEL,
                               "use-markup", TRUE,
                               "label", header_text,
                               "xalign", 0.0,
                               NULL);
    g_free(header_text);

    /* do multiplier and offset widgets */
    config_dialog->multiplier_adjust = g_object_new(GTK_TYPE_ADJUSTMENT,
                          "value", 1.0,
                          "lower", 0.001,
                          "upper", 1000.0,
                          "step-increment", 0.1,
                          "page-increment", 1.0,
                          "page-size", 1.0,
                          NULL);

    config_dialog->multiplier_spinbutton = g_object_new(GTK_TYPE_SPIN_BUTTON,
                                         "adjustment", config_dialog->multiplier_adjust,
                                         "digits", VALUE_DECIMAL_PLACES,
                                         "value", multiplier,
                                         "width-chars", SPINBUTTON_WIDTH_CHARS,
                                         NULL);

    gtk_widget_set_halign (GTK_WIDGET(config_dialog->multiplier_spinbutton), GTK_ALIGN_START);
    gtk_widget_set_valign (GTK_WIDGET(config_dialog->multiplier_spinbutton), GTK_ALIGN_START);

    config_dialog->multiplier_label = g_object_new(GTK_TYPE_LABEL,
                                                   "label", _("Sensor value _multiplier"),
                                                   "mnemonic-widget", config_dialog->multiplier_spinbutton,
                                                   "use-underline", TRUE,
                                                   "xalign", 0.0,
                                                   NULL);

    g_signal_connect(config_dialog->multiplier_spinbutton, "value-changed", G_CALLBACK(sensor_config_dialog_multiplier_changed), config_dialog);

    config_dialog->offset_adjust = g_object_new(GTK_TYPE_ADJUSTMENT,
                                 "value", 0.0,
                                 "lower", -1000.000,
                                 "upper", 1000.000,
                                 "step-increment", 0.01,
                                 "page-increment", 1.0,
                                 "page-size", 1.0,
                                 NULL);

    config_dialog->offset_spinbutton = g_object_new(GTK_TYPE_SPIN_BUTTON,
                                     "adjustment", config_dialog->offset_adjust,
                                     "digits", VALUE_DECIMAL_PLACES,
                                     "value", (gdouble)offset,
                                     "width-chars", SPINBUTTON_WIDTH_CHARS,
                                     NULL);

    gtk_widget_set_halign (GTK_WIDGET(config_dialog->offset_spinbutton), GTK_ALIGN_START);
    gtk_widget_set_valign (GTK_WIDGET(config_dialog->offset_spinbutton), GTK_ALIGN_START);

    config_dialog->offset_label = g_object_new(GTK_TYPE_LABEL,
                                "label", _("Sensor value _offset"),
                                "mnemonic-widget", config_dialog->offset_spinbutton,
                                "use-underline", TRUE,
                                "xalign", 0.0,
                                NULL);

    g_signal_connect(config_dialog->offset_spinbutton, "value-changed", G_CALLBACK(sensor_config_dialog_offset_changed), config_dialog);

    /* now do alarm widgets */
    header_text = g_markup_printf_escaped("<b>%s</b>", _("Sensor Limits"));
    config_dialog->limits_header = g_object_new(GTK_TYPE_LABEL,
                                                "use-markup", TRUE,
                                                "label", header_text,
                                                "xalign", 0.0,
                                                NULL);
    g_free(header_text);

    config_dialog->low_value_adjust = g_object_new(GTK_TYPE_ADJUSTMENT,
                                    "value", 0.0,
                                    "lower", -100000.0,
                                    "upper", 100000.0,
                                    "step-increment", 1.0,
                                    "page-increment", 10.0,
                                    "page-size", 100.0,
                                    NULL);

    config_dialog->low_value_spinbutton = g_object_new(GTK_TYPE_SPIN_BUTTON,
                                                       "adjustment", config_dialog->low_value_adjust,
                                                       "digits", VALUE_DECIMAL_PLACES,
                                                       "value", low_value,
                                                       "width-chars", SPINBUTTON_WIDTH_CHARS,
                                                       NULL);

    gtk_widget_set_halign (GTK_WIDGET(config_dialog->low_value_spinbutton), GTK_ALIGN_START);
    gtk_widget_set_valign (GTK_WIDGET(config_dialog->low_value_spinbutton), GTK_ALIGN_START);

    config_dialog->low_value_label = g_object_new(GTK_TYPE_LABEL,
                                                  "label", _("Sensor _low value"),
                                                  "mnemonic-widget", config_dialog->low_value_spinbutton,
                                                  "use-underline", TRUE,
                                                  "xalign", 0.0,
                                                  NULL);

    g_signal_connect(config_dialog->low_value_spinbutton, "value-changed", G_CALLBACK(sensor_config_dialog_low_value_changed), config_dialog);

    config_dialog->high_value_adjust = g_object_new(GTK_TYPE_ADJUSTMENT,
                                                  "value", 0.0,
                                                  "lower", -100000.0,
                                                  "upper", 100000.0,
                                                  "step-increment", 1.0,
                                                  "page-increment", 10.0,
                                                  "page-size", 100.0,
                                                  NULL);

    config_dialog->high_value_spinbutton = g_object_new(GTK_TYPE_SPIN_BUTTON,
                                                        "adjustment", config_dialog->high_value_adjust,
                                                        "digits", VALUE_DECIMAL_PLACES,
                                                        "value", high_value,
                                                        "width-chars", SPINBUTTON_WIDTH_CHARS,
                                                        NULL);

    gtk_widget_set_halign (GTK_WIDGET(config_dialog->high_value_spinbutton), GTK_ALIGN_START);
    gtk_widget_set_valign (GTK_WIDGET(config_dialog->high_value_spinbutton), GTK_ALIGN_START);

    config_dialog->high_value_label = g_object_new(GTK_TYPE_LABEL,
                                                   "label", _("Sensor _high value"),
                                                   "mnemonic-widget", config_dialog->high_value_spinbutton,
                                                   "use-underline", TRUE,
                                                   "xalign", 0.0,
                                                   NULL);

    g_signal_connect(config_dialog->high_value_spinbutton, "value-changed", G_CALLBACK(sensor_config_dialog_high_value_changed), config_dialog);

    header_text = g_markup_printf_escaped("<b>%s</b>", _("Alarm"));
    config_dialog->alarm_header = g_object_new(GTK_TYPE_LABEL,
                                               "use-markup", TRUE,
                                               "label", header_text,
                                               "xalign", 0.0,
                                               NULL);
    g_free(header_text);

    config_dialog->alarm_timeout_adjust = g_object_new(GTK_TYPE_ADJUSTMENT,
                                        "value", 0.0,
                                        "lower", 0.0,
                                        "upper", 10000.0,
                                        "step-increment", 1.0,
                                        "page-increment", 10.0,
                                        "page-size", 100.0,
                                        NULL);

    config_dialog->alarm_timeout_spinbutton = g_object_new(GTK_TYPE_SPIN_BUTTON,
                                                           "adjustment", config_dialog->alarm_timeout_adjust,
                                                           "digits", 0,
                                                           "value", (gdouble)alarm_timeout,
                                                           "width-chars", SPINBUTTON_WIDTH_CHARS,
                                                           "sensitive", alarm_enable,
                                                           NULL);

    gtk_widget_set_halign (GTK_WIDGET(config_dialog->alarm_timeout_spinbutton), GTK_ALIGN_START);
    gtk_widget_set_valign (GTK_WIDGET(config_dialog->alarm_timeout_spinbutton), GTK_ALIGN_START);

    config_dialog->alarm_timeout_label = g_object_new(GTK_TYPE_LABEL,
                                                      "label", _("Alarm _repeat interval (secs)"),
                                                      "mnemonic-widget", config_dialog->alarm_timeout_spinbutton,
                                                      "use-underline", TRUE,
                                                      "xalign", 0.0,
                                                      "sensitive", alarm_enable,
                                                      NULL);

    g_signal_connect(config_dialog->alarm_timeout_spinbutton, "value-changed", G_CALLBACK(sensor_config_dialog_alarm_timeout_changed), config_dialog);

    config_dialog->low_alarm_command_entry = g_object_new(GTK_TYPE_ENTRY,
                                                          "text", low_alarm_command,
                                                          "width-chars", 25,
                                                          "sensitive", alarm_enable,
                                                          NULL);

    g_free(low_alarm_command);

    config_dialog->low_alarm_command_label = g_object_new(GTK_TYPE_LABEL,
                                                      "use-underline", TRUE,
                                                      "label", _("Lo_w alarm command"),
                                                      "mnemonic-widget", config_dialog->low_alarm_command_entry,
                                                      "xalign", 0.0,
                                                      "sensitive", alarm_enable,
                                                      NULL);

    g_signal_connect(config_dialog->low_alarm_command_entry,
                     "changed",
                     G_CALLBACK(sensor_config_dialog_low_alarm_command_edited),
                     config_dialog);

    config_dialog->high_alarm_command_entry = g_object_new(GTK_TYPE_ENTRY,
                                                          "text", high_alarm_command,
                                                          "width-chars", 25,
                                                          "sensitive", alarm_enable,
                                                          NULL);

    g_free(high_alarm_command);

    config_dialog->high_alarm_command_label = g_object_new(GTK_TYPE_LABEL,
                                                      "use-underline", TRUE,
                                                      "label", _("Hi_gh alarm command"),
                                                      "mnemonic-widget", config_dialog->high_alarm_command_entry,
                                                      "xalign", 0.0,
                                                      "sensitive", alarm_enable,
                                                      NULL);

    g_signal_connect(config_dialog->high_alarm_command_entry,
                     "changed",
                     G_CALLBACK(sensor_config_dialog_high_alarm_command_edited),
                     config_dialog);

    config_dialog->alarm_enable_checkbutton = g_object_new(GTK_TYPE_CHECK_BUTTON,
                                            "use-underline", TRUE,
                                            "label", _("_Enable alarm"),
                                            "active", alarm_enable,
                                            "xalign", 0.0,
                                            NULL);

    gtk_widget_set_halign (GTK_WIDGET(config_dialog->alarm_enable_checkbutton), GTK_ALIGN_START);
    gtk_widget_set_valign (GTK_WIDGET(config_dialog->alarm_enable_checkbutton), GTK_ALIGN_START);

    g_signal_connect(config_dialog->alarm_enable_checkbutton, "toggled", G_CALLBACK(sensor_config_dialog_alarm_toggled), config_dialog);

    config_dialog->size_group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
    gtk_size_group_add_widget(config_dialog->size_group, GTK_WIDGET(config_dialog->multiplier_spinbutton));
    gtk_size_group_add_widget(config_dialog->size_group, GTK_WIDGET(config_dialog->offset_spinbutton));
    gtk_size_group_add_widget(config_dialog->size_group, GTK_WIDGET(config_dialog->low_value_spinbutton));
    gtk_size_group_add_widget(config_dialog->size_group, GTK_WIDGET(config_dialog->high_value_spinbutton));
    gtk_size_group_add_widget(config_dialog->size_group, GTK_WIDGET(config_dialog->alarm_timeout_spinbutton));
    gtk_size_group_add_widget(config_dialog->size_group, GTK_WIDGET(config_dialog->icon_type_combo_box));
    gtk_size_group_add_widget(config_dialog->size_group, GTK_WIDGET(config_dialog->graph_color_button));
    g_object_unref(config_dialog->size_group);

    config_dialog->grid = g_object_new(GTK_TYPE_GRID,
                                         "column-spacing", 5,
                                         "row-homogeneous", FALSE,
                                         "column-homogeneous", FALSE,
                                         "row-spacing", 6,
                                         "column-spacing", 12,
                                         NULL);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->scale_header),
                    0, 0, 2, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->multiplier_label),
                    1, 1, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->multiplier_spinbutton),
                    2, 1, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->offset_label),
                    1, 2, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->offset_spinbutton),
                    2, 2, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->limits_header),
                    0, 3, 2, 1);

    /* now pack alarm widgets */
    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->low_value_label),
                    1, 4, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->low_value_spinbutton),
                    2, 4, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->high_value_label),
                    1, 5, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->high_value_spinbutton),
                    2, 5, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->alarm_header),
                    0, 6, 2, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->alarm_enable_checkbutton),
                    1, 7, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->alarm_timeout_label),
                    1, 8, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->alarm_timeout_spinbutton),
                    2, 8, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->low_alarm_command_label),
                    1, 9, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->low_alarm_command_entry),
                    2, 9, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->high_alarm_command_label),
                    1, 10, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->high_alarm_command_entry),
                    2, 10, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->icon_header),
                    0, 11, 2, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->icon_type_label),
                    1, 12, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->icon_type_combo_box),
                    2, 12, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->graph_header),
                    0, 13, 2, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->graph_color_label),
                    1, 14, 1, 1);

    gtk_grid_attach(config_dialog->grid,
                    GTK_WIDGET(config_dialog->graph_color_button),
                    2, 14, 1, 1);

    content_area = gtk_dialog_get_content_area (config_dialog->dialog);
    gtk_box_pack_start(GTK_BOX(content_area), GTK_WIDGET(config_dialog->grid), FALSE, FALSE, 0);
    gtk_widget_show_all(GTK_WIDGET(config_dialog->dialog));

}
