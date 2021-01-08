/*
 * Copyright (C) 2020 MATE Development Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include "backend.h"
#include "netspeed-preferences.h"

struct _NetspeedPreferences
{
  GtkDialog parent;

  NetspeedApplet *netspeed;

  GSettings *settings;
  GtkWidget *network_device_combo;
  GtkWidget *show_all_addresses_checkbutton;
  GtkWidget *show_sum_checkbutton;
  GtkWidget *show_bits_checkbutton;
  GtkWidget *show_icon_checkbutton;
  GtkWidget *show_quality_icon_checkbutton;
  GtkWidget *change_icon_checkbutton;

  GList *devices;
};

typedef enum
{
  PROP_NETSPEED_APPLET = 1,
  N_PROPERTIES
} NetspeedPreferencesProperty;

G_DEFINE_TYPE (NetspeedPreferences, netspeed_preferences, GTK_TYPE_DIALOG)

static void
netspeed_preferences_init (NetspeedPreferences *preferences)
{
  gtk_widget_init_template (GTK_WIDGET (preferences));
}

static void
netspeed_preferences_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  NetspeedPreferences *self = NETSPEED_PREFERENCES (object);

  switch ((NetspeedPreferencesProperty) property_id) {
    case PROP_NETSPEED_APPLET:
      self->netspeed = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
netspeed_preferences_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  NetspeedPreferences *self = NETSPEED_PREFERENCES (object);

  switch ((NetspeedPreferencesProperty) property_id) {
    case PROP_NETSPEED_APPLET:
      g_value_set_pointer (value, self->netspeed);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
netspeed_preferences_response (GtkDialog *dialog,
                               gint       response_id)
{
  NetspeedPreferences *preferences;

  preferences = NETSPEED_PREFERENCES (dialog);

  switch (response_id) {
    case GTK_RESPONSE_HELP:
      netspeed_applet_display_help (GTK_WIDGET (dialog),
                                    "netspeed_applet-settings");
      break;
    default:
      gtk_widget_destroy (GTK_WIDGET (preferences));
  }
}

static void
on_network_device_combo_changed (GtkComboBox         *combo,
                                 NetspeedPreferences *preferences)
{
  gint active;
  gboolean old_auto_change_device, auto_change_device;

  active = gtk_combo_box_get_active (combo);
  g_assert (active > -1);

  old_auto_change_device = auto_change_device = g_settings_get_boolean (preferences->settings,
                                                                        "auto-change-device");
  if (0 == active) {
    if (auto_change_device)
      return;
    auto_change_device = TRUE;
  } else {
    const gchar *current_device_name;
    const gchar *selected_device_name;

    current_device_name = netspeed_applet_get_current_device_name (preferences->netspeed);
    auto_change_device = FALSE;
    selected_device_name = g_list_nth_data (preferences->devices, (guint) (active - 1));
    if (!g_strcmp0 (selected_device_name, current_device_name))
      return;
    g_settings_set_string (preferences->settings,
                           "device", selected_device_name);
  }
  if (old_auto_change_device != auto_change_device)
    g_settings_set_boolean (preferences->settings,
                            "auto-change-device", auto_change_device);
}

static void
netspeed_preferences_finalize (GObject *object)
{
  NetspeedPreferences *preferences = NETSPEED_PREFERENCES (object);

  g_list_free_full (preferences->devices, g_free);
  G_OBJECT_CLASS (netspeed_preferences_parent_class)->finalize (object);
}

static void
netspeed_preferences_class_init (NetspeedPreferencesClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->finalize = netspeed_preferences_finalize;
  object_class->set_property = netspeed_preferences_set_property;
  object_class->get_property = netspeed_preferences_get_property;

  dialog_class->response = netspeed_preferences_response;

  g_object_class_install_property (object_class,
                                   PROP_NETSPEED_APPLET,
                                   g_param_spec_pointer ("netspeed-applet",
                                                         "Netspeed Applet",
                                                         "The Netspeed Applet",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  gtk_widget_class_set_template_from_resource (widget_class, NETSPEED_RESOURCE_PATH "netspeed-preferences.ui");

  gtk_widget_class_bind_template_child (widget_class, NetspeedPreferences, network_device_combo);
  gtk_widget_class_bind_template_child (widget_class, NetspeedPreferences, show_all_addresses_checkbutton);
  gtk_widget_class_bind_template_child (widget_class, NetspeedPreferences, show_sum_checkbutton);
  gtk_widget_class_bind_template_child (widget_class, NetspeedPreferences, show_bits_checkbutton);
  gtk_widget_class_bind_template_child (widget_class, NetspeedPreferences, show_icon_checkbutton);
  gtk_widget_class_bind_template_child (widget_class, NetspeedPreferences, show_quality_icon_checkbutton);
  gtk_widget_class_bind_template_child (widget_class, NetspeedPreferences, change_icon_checkbutton);

  gtk_widget_class_bind_template_callback (widget_class, on_network_device_combo_changed);
}

static void
fill_device_combo (NetspeedPreferences *preferences, GSettings *settings)
{
  GList *ptr;
  int i, active = -1;
  const gchar *current_device_name;
  gboolean auto_change_device;

  /* Default means device with default route set */
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (preferences->network_device_combo),
                                  _("Default"));
  ptr = preferences->devices = get_available_devices ();
  current_device_name = netspeed_applet_get_current_device_name (preferences->netspeed);
  auto_change_device = g_settings_get_boolean (settings, "auto-change-device");
  for (i = 0; ptr; ptr = g_list_next (ptr)) {
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (preferences->network_device_combo),
                                    ptr->data);
    if (g_str_equal (ptr->data, current_device_name))
      active = (i + 1);
    ++i;
  }
  if (active < 0 || auto_change_device)
    active = 0;
  gtk_combo_box_set_active (GTK_COMBO_BOX (preferences->network_device_combo), active);
}

GtkWidget *
netspeed_preferences_new (NetspeedApplet *netspeed)
{
  NetspeedPreferences *preferences;
  GSettings *settings;

  preferences = g_object_new (NETSPEED_TYPE_PREFERENCES,
                              "netspeed-applet", netspeed,
                              NULL);

  preferences->settings = settings = netspeed_applet_get_settings (preferences->netspeed);

  fill_device_combo (preferences, settings);

  g_settings_bind (settings, "show-all-addresses",
                   preferences->show_all_addresses_checkbutton, "active",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, "show-sum",
                   preferences->show_sum_checkbutton, "active",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, "show-bits",
                   preferences->show_bits_checkbutton, "active",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, "show-icon",
                   preferences->show_icon_checkbutton, "active",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, "show-quality-icon",
                   preferences->show_quality_icon_checkbutton, "active",
                   G_SETTINGS_BIND_DEFAULT);

  g_settings_bind (settings, "change-icon",
                   preferences->change_icon_checkbutton, "active",
                   G_SETTINGS_BIND_DEFAULT);

  return GTK_WIDGET (preferences);
}
