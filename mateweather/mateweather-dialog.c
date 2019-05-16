/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Main status dialog
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <gio/gio.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "mateweather.h"
#include "mateweather-applet.h"
#include "mateweather-pref.h"
#include "mateweather-dialog.h"

struct _MateWeatherDialogPrivate {
	GtkWidget* cond_location;
	GtkWidget* cond_update;
	GtkWidget* cond_cond;
	GtkWidget* cond_sky;
	GtkWidget* cond_temp;
	GtkWidget* cond_dew;
	GtkWidget* cond_humidity;
	GtkWidget* cond_wind;
	GtkWidget* cond_pressure;
	GtkWidget* cond_vis;
	GtkWidget* cond_apparent;
	GtkWidget* cond_sunrise;
	GtkWidget* cond_sunset;
	GtkWidget* cond_image;
	GtkWidget* forecast_text;
	GtkWidget* radar_image;

	MateWeatherApplet* applet;
};

enum {
	PROP_0,
	PROP_MATEWEATHER_APPLET,
};

G_DEFINE_TYPE_WITH_PRIVATE (MateWeatherDialog, mateweather_dialog, GTK_TYPE_DIALOG);

#define MONOSPACE_FONT_SCHEMA  "org.mate.interface"
#define MONOSPACE_FONT_KEY     "monospace-font-name"

static void mateweather_dialog_save_geometry(MateWeatherDialog* dialog)
{
	GSettings* settings;
	int w, h;

	settings = dialog->priv->applet->settings;

	gtk_window_get_size(GTK_WINDOW(dialog), &w, &h);

#if 0
	/* FIXME those keys are not in org.mate.weather! */
	g_settings_set_int (settings, "dialog-width", w);
	g_settings_set_int (settings, "dialog-height", h);
#endif
}

static void mateweather_dialog_load_geometry(MateWeatherDialog* dialog)
{
	GSettings* settings;
	int w, h;

	settings = dialog->priv->applet->settings;

#if 0
	/* FIXME those keys are not in org.mate.weather! */
	w = g_settings_get_int (settings, "dialog-width");
	h = g_settings_get_int (settings, "dialog-height");

	if (w > 0 && h > 0)
	{
		gtk_window_resize(GTK_WINDOW(dialog), w, h);
	}
#endif
}

static void response_cb(MateWeatherDialog* dialog, gint id, gpointer data)
{
    if (id == GTK_RESPONSE_OK) {
	mateweather_update (dialog->priv->applet);

	mateweather_dialog_update (dialog);
    } else {
        gtk_widget_destroy (GTK_WIDGET(dialog));
    }
}

static void link_cb(GtkButton* button, gpointer data)
{
    gtk_show_uri_on_window (NULL,
                            "http://www.weather.com/",
                            gtk_get_current_event_time (),
                            NULL);
}

static gchar* replace_multiple_new_lines(gchar* s)
{
	gchar *prev_s = s;
	gint count = 0;
	gint i;

	if (s == NULL) {
		return s;
	}

	for (;*s != '\0';s++) {

		count = 0;

		if (*s == '\n') {
			s++;
			while (*s == '\n' || *s == ' ') {
				count++;
				s++;
			}
		}
		for (i = count; i > 1; i--) {
			*(s - i) = ' ';
		}
	}
	return prev_s;
}

static void mateweather_dialog_create(MateWeatherDialog* dialog)
{
  MateWeatherDialogPrivate *priv;
  MateWeatherApplet *gw_applet;

  GtkWidget *weather_vbox;
  GtkWidget *weather_notebook;
  GtkWidget *cond_hbox;
  GtkWidget *cond_grid;
  GtkWidget *cond_location_lbl;
  GtkWidget *cond_update_lbl;
  GtkWidget *cond_temp_lbl;
  GtkWidget *cond_cond_lbl;
  GtkWidget *cond_sky_lbl;
  GtkWidget *cond_wind_lbl;
  GtkWidget *cond_humidity_lbl;
  GtkWidget *cond_pressure_lbl;
  GtkWidget *cond_vis_lbl;
  GtkWidget *cond_dew_lbl;
  GtkWidget *cond_apparent_lbl;
  GtkWidget *cond_sunrise_lbl;
  GtkWidget *cond_sunset_lbl;
  GtkWidget *cond_vbox;
  GtkWidget *current_note_lbl;
  GtkWidget *forecast_note_lbl;
  GtkWidget *radar_note_lbl;
  GtkWidget *radar_vbox;
  GtkWidget *radar_link_btn;
  GtkWidget *radar_link_box;
  GtkWidget *forecast_hbox;
  GtkWidget *ebox;
  GtkWidget *scrolled_window;
  GtkWidget *imagescroll_window;

  priv = dialog->priv;
  gw_applet = priv->applet;

  g_object_set (dialog, "destroy-with-parent", TRUE, NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Details"));
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
  			  _("_Update"), GTK_RESPONSE_OK,
  			  "gtk-close", GTK_RESPONSE_CLOSE,
			  NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

  if (gw_applet->mateweather_pref.radar_enabled)
      gtk_window_set_default_size (GTK_WINDOW (dialog), 570,440);
  else
      gtk_window_set_default_size (GTK_WINDOW (dialog), 590, 340);

  gtk_window_set_screen (GTK_WINDOW (dialog),
			 gtk_widget_get_screen (GTK_WIDGET (gw_applet->applet)));
  mateweather_dialog_load_geometry (dialog);

  /* Must come after load geometry, otherwise it will get reset. */
  gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

  weather_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_widget_show (weather_vbox);

  weather_notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (weather_notebook), 5);
  gtk_widget_show (weather_notebook);
  gtk_box_pack_start (GTK_BOX (weather_vbox), weather_notebook, TRUE, TRUE, 0);

  cond_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_show (cond_hbox);
  gtk_container_add (GTK_CONTAINER (weather_notebook), cond_hbox);
  gtk_container_set_border_width (GTK_CONTAINER (cond_hbox), 4);

  cond_grid = gtk_grid_new ();
  gtk_widget_show (cond_grid);
  gtk_box_pack_start (GTK_BOX (cond_hbox), cond_grid, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (cond_grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (cond_grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (cond_grid), 12);

  cond_location_lbl = gtk_label_new (_("City:"));
  gtk_widget_show (cond_location_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_location_lbl, 0, 0, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_location_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_location_lbl), 0.0);

  cond_update_lbl = gtk_label_new (_("Last update:"));
  gtk_widget_show (cond_update_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_update_lbl, 0, 1, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_update_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_update_lbl), 0.0);

  cond_cond_lbl = gtk_label_new (_("Conditions:"));
  gtk_widget_show (cond_cond_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_cond_lbl, 0, 2, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_cond_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_cond_lbl), 0.0);

  cond_sky_lbl = gtk_label_new (_("Sky:"));
  gtk_widget_show (cond_sky_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_sky_lbl, 0, 3, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_sky_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_sky_lbl), 0.0);

  cond_temp_lbl = gtk_label_new (_("Temperature:"));
  gtk_widget_show (cond_temp_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_temp_lbl, 0, 4, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_temp_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_temp_lbl), 0.0);

  cond_apparent_lbl = gtk_label_new (_("Feels like:"));
  gtk_widget_show (cond_apparent_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_apparent_lbl, 0, 5, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_apparent_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_apparent_lbl), 0.0);

  cond_dew_lbl = gtk_label_new (_("Dew point:"));
  gtk_widget_show (cond_dew_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_dew_lbl, 0, 6, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_dew_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_dew_lbl), 0.0);

  cond_humidity_lbl = gtk_label_new (_("Relative humidity:"));
  gtk_widget_show (cond_humidity_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_humidity_lbl, 0, 7, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_humidity_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_humidity_lbl), 0.0);

  cond_wind_lbl = gtk_label_new (_("Wind:"));
  gtk_widget_show (cond_wind_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_wind_lbl, 0, 8, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_wind_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_wind_lbl), 0.0);

  cond_pressure_lbl = gtk_label_new (_("Pressure:"));
  gtk_widget_show (cond_pressure_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_pressure_lbl, 0, 9, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_pressure_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_pressure_lbl), 0.0);

  cond_vis_lbl = gtk_label_new (_("Visibility:"));
  gtk_widget_show (cond_vis_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_vis_lbl, 0, 10, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_vis_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_vis_lbl), 0.0);

  cond_sunrise_lbl = gtk_label_new (_("Sunrise:"));
  gtk_widget_show (cond_sunrise_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_sunrise_lbl, 0, 11, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_sunrise_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_sunrise_lbl), 0.0);

  cond_sunset_lbl = gtk_label_new (_("Sunset:"));
  gtk_widget_show (cond_sunset_lbl);
  gtk_grid_attach (GTK_GRID (cond_grid), cond_sunset_lbl, 0, 12, 1, 1);
  gtk_label_set_justify (GTK_LABEL (cond_sunset_lbl), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (cond_sunset_lbl), 0.0);

  priv->cond_location = gtk_label_new ("");
  gtk_widget_show (priv->cond_location);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_location, 1, 0, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_location), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_location), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_location), 0.0);

  priv->cond_update = gtk_label_new ("");
  gtk_widget_show (priv->cond_update);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_update, 1, 1, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_update), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_update), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_update), 0.0);

  priv->cond_cond = gtk_label_new ("");
  gtk_widget_show (priv->cond_cond);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_cond, 1, 2, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_cond), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_cond), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_cond), 0.0);

  priv->cond_sky = gtk_label_new ("");
  gtk_widget_show (priv->cond_sky);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_sky, 1, 3, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_sky), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_sky), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_sky), 0.0);

  priv->cond_temp = gtk_label_new ("");
  gtk_widget_show (priv->cond_temp);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_temp, 1, 4, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_temp), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_temp), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_temp), 0.0);

  priv->cond_apparent = gtk_label_new ("");
  gtk_widget_show (priv->cond_apparent);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_apparent, 1, 5, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_apparent), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_apparent), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_apparent), 0.0);

  priv->cond_dew = gtk_label_new ("");
  gtk_widget_show (priv->cond_dew);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_dew, 1, 6, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_dew), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_dew), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_dew), 0.0);

  priv->cond_humidity = gtk_label_new ("");
  gtk_widget_show (priv->cond_humidity);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_humidity, 1, 7, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_humidity), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_humidity), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_humidity), 0.0);

  priv->cond_wind = gtk_label_new ("");
  gtk_widget_show (priv->cond_wind);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_wind, 1, 8, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_wind), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_wind), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_wind), 0.0);

  priv->cond_pressure = gtk_label_new ("");
  gtk_widget_show (priv->cond_pressure);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_pressure, 1, 9, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_pressure), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_pressure), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_pressure), 0.0);

  priv->cond_vis = gtk_label_new ("");
  gtk_widget_show (priv->cond_vis);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_vis, 1, 10, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_vis), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_vis), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_vis), 0.0);

  priv->cond_sunrise = gtk_label_new ("");
  gtk_widget_show (priv->cond_sunrise);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_sunrise, 1, 11, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_sunrise), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_sunrise), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_sunrise), 0.0);

  priv->cond_sunset = gtk_label_new ("");
  gtk_widget_show (priv->cond_sunset);
  gtk_grid_attach (GTK_GRID (cond_grid), priv->cond_sunset, 1, 12, 1, 1);
  gtk_label_set_selectable (GTK_LABEL (priv->cond_sunset), TRUE);
  gtk_label_set_justify (GTK_LABEL (priv->cond_sunset), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign (GTK_LABEL (priv->cond_sunset), 0.0);

  cond_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_valign (cond_vbox, GTK_ALIGN_START);
  gtk_widget_set_vexpand (cond_vbox, TRUE);
  gtk_widget_show (cond_vbox);
  gtk_box_pack_end (GTK_BOX (cond_hbox), cond_vbox, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (cond_vbox), 2);

  priv->cond_image = gtk_image_new_from_icon_name ("stock-unknown", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (priv->cond_image);
  gtk_container_add (GTK_CONTAINER (cond_vbox), priv->cond_image);

  current_note_lbl = gtk_label_new (_("Current Conditions"));
  gtk_widget_show (current_note_lbl);
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (weather_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (weather_notebook), 0), current_note_lbl);

  if (gw_applet->mateweather_pref.location->zone_valid) {

      forecast_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_container_set_border_width (GTK_CONTAINER (forecast_hbox), 12);
      gtk_widget_show (forecast_hbox);

      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                           GTK_SHADOW_ETCHED_IN);

      priv->forecast_text = gtk_text_view_new ();
      set_access_namedesc (priv->forecast_text, _("Forecast Report"), _("See the ForeCast Details"));
      gtk_container_add (GTK_CONTAINER (scrolled_window), priv->forecast_text);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->forecast_text), FALSE);
      gtk_text_view_set_left_margin (GTK_TEXT_VIEW (priv->forecast_text), 6);
      gtk_widget_show (priv->forecast_text);
      gtk_widget_show (scrolled_window);
      gtk_box_pack_start (GTK_BOX (forecast_hbox), scrolled_window, TRUE, TRUE, 0);

      gtk_container_add (GTK_CONTAINER (weather_notebook), forecast_hbox);

      forecast_note_lbl = gtk_label_new (_("Forecast"));
      gtk_widget_show (forecast_note_lbl);
      gtk_notebook_set_tab_label (GTK_NOTEBOOK (weather_notebook), gtk_notebook_get_nth_page (GTK_NOTEBOOK (weather_notebook), 1), forecast_note_lbl);

  }

  if (gw_applet->mateweather_pref.radar_enabled) {

      radar_note_lbl = gtk_label_new_with_mnemonic (_("Radar Map"));
      gtk_widget_show (radar_note_lbl);

      radar_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_widget_show (radar_vbox);
      gtk_notebook_append_page (GTK_NOTEBOOK (weather_notebook), radar_vbox, radar_note_lbl);
      gtk_container_set_border_width (GTK_CONTAINER (radar_vbox), 6);

      priv->radar_image = gtk_image_new ();

      imagescroll_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (imagescroll_window),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (imagescroll_window),
                                      GTK_SHADOW_ETCHED_IN);

      ebox = gtk_event_box_new ();
      gtk_widget_show (ebox);

      gtk_container_add (GTK_CONTAINER (imagescroll_window),ebox);
      gtk_box_pack_start (GTK_BOX (radar_vbox), imagescroll_window, TRUE, TRUE, 0);
      gtk_widget_show (priv->radar_image);
      gtk_widget_show (imagescroll_window);

      gtk_container_add (GTK_CONTAINER (ebox), priv->radar_image);

      radar_link_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      gtk_widget_set_halign (radar_link_box, GTK_ALIGN_CENTER);
      gtk_widget_set_hexpand (radar_link_box, TRUE);
      gtk_widget_show (radar_link_box);
      gtk_box_pack_start (GTK_BOX (radar_vbox), radar_link_box, FALSE, FALSE, 0);

      radar_link_btn = gtk_button_new_with_mnemonic (_("_Visit Weather.com"));
      set_access_namedesc (radar_link_btn, _("Visit Weather.com"), _("Click to Enter Weather.com"));
      gtk_widget_set_size_request (radar_link_btn, 450, -2);
      gtk_widget_show (radar_link_btn);
      if (!g_settings_get_boolean (gw_applet->settings, "use-custom-radar-url"))
          gtk_container_add (GTK_CONTAINER (radar_link_box), radar_link_btn);

      g_signal_connect (G_OBJECT (radar_link_btn), "clicked",
                        G_CALLBACK (link_cb), NULL);

  }

  g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (response_cb), NULL);

}

static PangoFontDescription* get_system_monospace_font(void)
{
    PangoFontDescription *desc = NULL;
    GSettings *settings;
    char *name;

    settings = g_settings_new (MONOSPACE_FONT_SCHEMA);
    name = g_settings_get_string (settings, MONOSPACE_FONT_KEY);

    if (name) {
    	desc = pango_font_description_from_string (name);
    	g_free (name);
    }

    g_object_unref (settings);

    return desc;
}

static void
override_widget_font (GtkWidget            *widget,
                      PangoFontDescription *font)
{
    static gboolean provider_added = FALSE;
    static GtkCssProvider *provider;
    gchar          *css;
    gchar          *family;
    gchar          *weight;
    const gchar    *style;
    gchar          *size;

    family = g_strdup_printf ("font-family: %s;", pango_font_description_get_family (font));

    weight = g_strdup_printf ("font-weight: %d;", pango_font_description_get_weight (font));

    if (pango_font_description_get_style (font) == PANGO_STYLE_NORMAL)
        style = "font-style: normal;";
    else if (pango_font_description_get_style (font) == PANGO_STYLE_ITALIC)
        style = "font-style: italic;";
    else
        style = "font-style: oblique;";

    size = g_strdup_printf ("font-size: %d%s;",
                            pango_font_description_get_size (font) / PANGO_SCALE,
                            pango_font_description_get_size_is_absolute (font) ? "px" : "pt");
    if (!provider_added)
        provider = gtk_css_provider_new ();

    gtk_widget_set_name(GTK_WIDGET(widget), "MateWeatherAppletTextView");
    css = g_strdup_printf ("#MateWeatherAppletTextView { %s %s %s %s }", family, weight, style, size);
    gtk_css_provider_load_from_data (provider, css, -1, NULL);

    if (!provider_added) {
        gtk_style_context_add_provider_for_screen (gtk_widget_get_screen (widget),
                                                   GTK_STYLE_PROVIDER (provider),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        provider_added = TRUE;
    }

    g_free (css);
    g_free (family);
    g_free (weight);
    g_free (size);
}

void mateweather_dialog_update(MateWeatherDialog* dialog)
{
    MateWeatherDialogPrivate *priv;
    MateWeatherApplet *gw_applet;
    gchar *forecast;
    GtkTextBuffer *buffer;
    PangoFontDescription *font_desc;
    const gchar *icon_name;

    priv = dialog->priv;
    gw_applet = priv->applet;

    /* Check for parallel network update in progress */
    if(gw_applet->mateweather_info == NULL)
    	return;

    /* Update icon */
    icon_name = weather_info_get_icon_name (gw_applet->mateweather_info);
    gtk_image_set_from_icon_name (GTK_IMAGE (priv->cond_image),
                                  icon_name, GTK_ICON_SIZE_DIALOG);

    /* Update current condition fields and forecast */
    gtk_label_set_text(GTK_LABEL(priv->cond_location), weather_info_get_location_name(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_update), weather_info_get_update(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_cond), weather_info_get_conditions(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_sky), weather_info_get_sky(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_temp), weather_info_get_temp(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_apparent), weather_info_get_apparent(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_dew), weather_info_get_dew(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_humidity), weather_info_get_humidity(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_wind), weather_info_get_wind(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_pressure), weather_info_get_pressure(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_vis), weather_info_get_visibility(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_sunrise), weather_info_get_sunrise(gw_applet->mateweather_info));
    gtk_label_set_text(GTK_LABEL(priv->cond_sunset), weather_info_get_sunset(gw_applet->mateweather_info));

    /* Update forecast */
    if (gw_applet->mateweather_pref.location->zone_valid) {
	font_desc = get_system_monospace_font ();
	if (font_desc) {
            override_widget_font (priv->forecast_text, font_desc);
            pango_font_description_free (font_desc);
	}

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->forecast_text));
        forecast = g_strdup(weather_info_get_forecast(gw_applet->mateweather_info));
        if (forecast) {
            forecast = g_strstrip(replace_multiple_new_lines(forecast));
            gtk_text_buffer_set_text(buffer, forecast, -1);
            g_free(forecast);
        } else {
            gtk_text_buffer_set_text(buffer, _("Forecast not currently available for this location."), -1);
        }
    }

    /* Update radar map */
    if (gw_applet->mateweather_pref.radar_enabled) {
        GdkPixbufAnimation *radar;

	radar = weather_info_get_radar (gw_applet->mateweather_info);
        if (radar) {
            gtk_image_set_from_animation (GTK_IMAGE (priv->radar_image), radar);
        }
    }
}

static void mateweather_dialog_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    MateWeatherDialog *dialog = MATEWEATHER_DIALOG (object);

    switch (prop_id) {
	case PROP_MATEWEATHER_APPLET:
	    dialog->priv->applet = g_value_get_pointer (value);
	    break;
    }
}

static void mateweather_dialog_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    MateWeatherDialog *dialog = MATEWEATHER_DIALOG (object);

    switch (prop_id) {
	case PROP_MATEWEATHER_APPLET:
	    g_value_set_pointer (value, dialog->priv->applet);
	    break;
    }
}

static void mateweather_dialog_init(MateWeatherDialog* self)
{
    self->priv = mateweather_dialog_get_instance_private (self);
}

static GObject* mateweather_dialog_constructor(GType type, guint n_construct_params, GObjectConstructParam* construct_params)
{
    GObject *object;
    MateWeatherDialog *self;

    object = G_OBJECT_CLASS (mateweather_dialog_parent_class)->
        constructor (type, n_construct_params, construct_params);
    self = MATEWEATHER_DIALOG (object);

    mateweather_dialog_create (self);
    mateweather_dialog_update (self);

    return object;
}

GtkWidget* mateweather_dialog_new(MateWeatherApplet* applet)
{
	return g_object_new(MATEWEATHER_TYPE_DIALOG,
		"mateweather-applet", applet,
		NULL);
}

static void mateweather_dialog_unrealize(GtkWidget* widget)
{
    MateWeatherDialog* self = MATEWEATHER_DIALOG(widget);

    mateweather_dialog_save_geometry(self);

    GTK_WIDGET_CLASS(mateweather_dialog_parent_class)->unrealize(widget);
}

static void mateweather_dialog_class_init(MateWeatherDialogClass* klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    mateweather_dialog_parent_class = g_type_class_peek_parent (klass);

    object_class->set_property = mateweather_dialog_set_property;
    object_class->get_property = mateweather_dialog_get_property;
    object_class->constructor = mateweather_dialog_constructor;
    widget_class->unrealize = mateweather_dialog_unrealize;

    /* This becomes an OBJECT property when MateWeatherApplet is redone */
    g_object_class_install_property(object_class, PROP_MATEWEATHER_APPLET, g_param_spec_pointer ("mateweather-applet", "MateWeather Applet", "The MateWeather Applet", G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}
