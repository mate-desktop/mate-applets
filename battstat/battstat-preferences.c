/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* battstat        A MATE battery meter for laptops.
 * Copyright (C) 2000 by Jörgen Pehrson <jp@spektr.eu.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include <gio/gio.h>

#include "battstat-preferences.h"

enum {
  PROP_0,
  PROP_PROGRESS_DATA
};

struct _BattstatPreferences
{
  GtkDialog parent;

  GtkWidget *radio_text_1;
  GtkWidget *radio_text_2;
  GtkWidget *check_text;
  GtkWidget *check_text_ptr;
  GtkWidget *lowbatt_toggle;
  GtkWidget *full_toggle;
  GtkWidget *hbox_ptr;
  GtkWidget *combo_ptr;
  GtkWidget *spin_ptr;

  ProgressData *battstat;
};

G_DEFINE_TYPE (BattstatPreferences, battstat_preferences, GTK_TYPE_DIALOG);

static void
combo_ptr_cb (GtkWidget *combo_ptr,
              gpointer   data)
{
  BattstatPreferences *dialog = data;

  if (gtk_combo_box_get_active (GTK_COMBO_BOX (combo_ptr)))
    dialog->battstat->red_value_is_time = TRUE;
  else
    dialog->battstat->red_value_is_time = FALSE;

  g_settings_set_boolean (dialog->battstat->settings,
                          "red-value-is-time",
                          dialog->battstat->red_value_is_time);
}

static void
spin_ptr_cb (GtkWidget *spin_ptr,
             gpointer   data)
{
  BattstatPreferences *dialog = data;
  gdouble red_val = gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_ptr));

  dialog->battstat->red_val = (guint) red_val;
  /* automatically calculate orangle and yellow values from the
   * red value
   */
  dialog->battstat->orange_val = (guint) (ORANGE_MULTIPLIER * red_val);
  dialog->battstat->orange_val = MIN (dialog->battstat->orange_val, 100);

  dialog->battstat->yellow_val = (guint) (YELLOW_MULTIPLIER * red_val);
  dialog->battstat->yellow_val = MIN (dialog->battstat->yellow_val, 100);

  g_settings_set_int (dialog->battstat->settings,
                      "red-value",
                      dialog->battstat->red_val);
}

static void
show_text_toggled (GtkToggleButton *button,
                   gpointer         data)
{
  BattstatPreferences *dialog = data;
  gboolean show_text_active;

  show_text_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->check_text));
  if (show_text_active) {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_text_1)))
      dialog->battstat->showtext = APPLET_SHOW_TIME;
    else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->radio_text_2)))
      dialog->battstat->showtext = APPLET_SHOW_PERCENT;
    else
      dialog->battstat->showtext = APPLET_SHOW_NONE;
  } else {
    dialog->battstat->showtext = APPLET_SHOW_NONE;
  }

  dialog->battstat->refresh_label = TRUE;

  reconfigure_layout (dialog->battstat);

  g_settings_set_int (dialog->battstat->settings,
                      "show-text",
                      dialog->battstat->showtext);
}

static void
lowbatt_toggled (GtkToggleButton *button,
                 gpointer         data)
{
  BattstatPreferences *dialog = data;

  dialog->battstat->lowbattnotification = gtk_toggle_button_get_active (button);
}

static void
full_toggled (GtkToggleButton *button,
              gpointer         data)
{
  BattstatPreferences *dialog = data;

  dialog->battstat->fullbattnot = gtk_toggle_button_get_active (button);
}

static void
response_cb (GtkDialog *dialog,
             gint       id,
             gpointer   data)
{
  BattstatPreferences *self = data;

  if (id == GTK_RESPONSE_HELP)
    battstat_show_help (self->battstat, "battstat-appearance");
  else
    gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
battstat_preferences_update (BattstatPreferences *dialog)
{
  gboolean check_text_active = TRUE;

  if (!g_settings_is_writable (dialog->battstat->settings, "low-battery-notification")) {
    gtk_widget_set_sensitive (dialog->lowbatt_toggle, FALSE);
  }

  gtk_widget_set_sensitive (dialog->hbox_ptr, dialog->battstat->lowbattnotification);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->spin_ptr), dialog->battstat->red_val);

  if (dialog->battstat->red_value_is_time)
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->combo_ptr), 1);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->combo_ptr), 0);

  if (!g_settings_is_writable (dialog->battstat->settings, "full-battery-notification"))
    gtk_widget_set_sensitive (dialog->full_toggle, FALSE);

  if (dialog->battstat->fullbattnot)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->full_toggle), TRUE);

  if (dialog->battstat->lowbattnotification)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->lowbatt_toggle), TRUE);

  if (!g_settings_is_writable (dialog->battstat->settings, "show-text"))
  {
    gtk_widget_set_sensitive (dialog->check_text, FALSE);
    check_text_active = FALSE;
  }
  switch (dialog->battstat->showtext) {
    case APPLET_SHOW_PERCENT:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->check_text), TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->radio_text_2), TRUE);
      break;
    case APPLET_SHOW_TIME:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->check_text), TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->radio_text_1), TRUE);
      break;
    case APPLET_SHOW_NONE:
    default:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->check_text), FALSE);
      check_text_active = FALSE;
  }
  gtk_widget_set_sensitive (dialog->check_text_ptr, check_text_active);
}

static void
battstat_preferences_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BattstatPreferences *dialog;

  dialog = BATTSTAT_PREFERENCES (object);

  switch (prop_id) {
    case PROP_PROGRESS_DATA:
      dialog->battstat = g_value_get_pointer (value);
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
battstat_preferences_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BattstatPreferences *dialog;

  dialog = BATTSTAT_PREFERENCES (object);

  switch (prop_id) {
    case PROP_PROGRESS_DATA:
      g_value_set_pointer (value, dialog->battstat);
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
battstat_preferences_init (BattstatPreferences *dialog)
{
  gtk_widget_init_template (GTK_WIDGET (dialog));

  g_object_bind_property (dialog->check_text, "active",
                          dialog->check_text_ptr, "sensitive",
                          G_BINDING_DEFAULT);

  g_object_bind_property (dialog->lowbatt_toggle, "active",
                          dialog->hbox_ptr, "sensitive",
                          G_BINDING_DEFAULT);
}

static GObject*
battstat_preferences_constructor (GType                  type,
                                  guint                  n_construct_params,
                                  GObjectConstructParam *construct_params)
{
  GObject *object;
  BattstatPreferences *self;

  object = G_OBJECT_CLASS (battstat_preferences_parent_class)->
    constructor (type, n_construct_params, construct_params);
  self = BATTSTAT_PREFERENCES (object);

  battstat_preferences_update (self);

  g_settings_bind (self->battstat->settings, "low-battery-notification",
                   self->lowbatt_toggle, "active",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (self->battstat->settings, "full-battery-notification",
                   self->full_toggle, "active",
                   G_SETTINGS_BIND_DEFAULT);

  return object;
}

GtkWidget*
battstat_preferences_new (ProgressData *battstat)
{
  return g_object_new (BATTSTAT_TYPE_PREFERENCES,
                       "progress-data", battstat,
                       NULL);
}

static void
battstat_preferences_class_init (BattstatPreferencesClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = battstat_preferences_set_property;
  object_class->get_property = battstat_preferences_get_property;
  object_class->constructor  = battstat_preferences_constructor;

  g_object_class_install_property (object_class,
                                   PROP_PROGRESS_DATA,
                                   g_param_spec_pointer ("progress-data",
                                                         "Progress Data",
                                                         "The Progress Data",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  gtk_widget_class_set_template_from_resource (widget_class,
                                               BATTSTAT_RESOURCE_PATH "battstat-preferences.ui");

  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, check_text);
  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, check_text_ptr);
  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, radio_text_1);
  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, radio_text_2);
  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, lowbatt_toggle);
  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, full_toggle);
  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, hbox_ptr);
  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, combo_ptr);
  gtk_widget_class_bind_template_child (widget_class, BattstatPreferences, spin_ptr);

  gtk_widget_class_bind_template_callback (widget_class, lowbatt_toggled);
  gtk_widget_class_bind_template_callback (widget_class, combo_ptr_cb);
  gtk_widget_class_bind_template_callback (widget_class, spin_ptr_cb);
  gtk_widget_class_bind_template_callback (widget_class, full_toggled);
  gtk_widget_class_bind_template_callback (widget_class, show_text_toggled);
  gtk_widget_class_bind_template_callback (widget_class, response_cb);
}
