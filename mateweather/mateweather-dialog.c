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
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <gio/gio.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "mateweather.h"
#include "mateweather-applet.h"
#include "mateweather-pref.h"
#include "mateweather-dialog.h"

struct _MateWeatherDialog {
    GtkDialog  parent;

    GtkWidget *weather_notebook;
    GtkWidget *cond_location;
    GtkWidget *cond_update;
    GtkWidget *cond_cond;
    GtkWidget *cond_sky;
    GtkWidget *cond_temp;
    GtkWidget *cond_dew;
    GtkWidget *cond_humidity;
    GtkWidget *cond_wind;
    GtkWidget *cond_pressure;
    GtkWidget *cond_vis;
    GtkWidget *cond_apparent;
    GtkWidget *cond_sunrise;
    GtkWidget *cond_sunset;
    GtkWidget *cond_image;
    GtkWidget *forecast_text;
    GtkWidget *radar_image;

    MateWeatherApplet *applet;
};

enum {
    PROP_0,
    PROP_MATEWEATHER_APPLET,
};

G_DEFINE_TYPE (MateWeatherDialog, mateweather_dialog, GTK_TYPE_DIALOG);

#define MONOSPACE_FONT_SCHEMA  "org.mate.interface"
#define MONOSPACE_FONT_KEY     "monospace-font-name"

static void
response_cb (MateWeatherDialog *dialog,
             gint               id,
             gpointer           data)
{
    if (id == GTK_RESPONSE_OK) {
        mateweather_update (dialog->applet);
        mateweather_dialog_update (dialog);
    } else {
        gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}

static void
link_cb (GtkButton *button,
         gpointer   data)
{
    gtk_show_uri_on_window (NULL,
                            "http://www.weather.com/",
                            gtk_get_current_event_time (),
                            NULL);
}

static gchar*
replace_multiple_new_lines(gchar* s)
{
	gchar *prev_s = s;
	gint count;
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

void
mateweather_dialog_update (MateWeatherDialog *dialog)
{
    WeatherInfo *info;
    gchar *forecast;
    GtkTextBuffer *buffer;
    PangoFontDescription *font_desc;
    const gchar *icon_name;

    /* Check for parallel network update in progress */
    if (dialog->applet->mateweather_info == NULL)
    	return;

    info = dialog->applet->mateweather_info;

    /* Update icon */
    icon_name = weather_info_get_icon_name (info);
    gtk_image_set_from_icon_name (GTK_IMAGE (dialog->cond_image),
                                  icon_name, GTK_ICON_SIZE_DIALOG);

    /* Update current condition fields and forecast */
    gtk_label_set_text (GTK_LABEL (dialog->cond_location), weather_info_get_location_name (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_update),   weather_info_get_update        (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_cond),     weather_info_get_conditions    (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_sky),      weather_info_get_sky           (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_temp),     weather_info_get_temp          (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_apparent), weather_info_get_apparent      (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_dew),      weather_info_get_dew           (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_humidity), weather_info_get_humidity      (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_wind),     weather_info_get_wind          (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_pressure), weather_info_get_pressure      (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_vis),      weather_info_get_visibility    (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_sunrise),  weather_info_get_sunrise       (info));
    gtk_label_set_text (GTK_LABEL (dialog->cond_sunset),   weather_info_get_sunset        (info));

    /* Update forecast */
    if (dialog->applet->mateweather_pref.location->zone_valid) {
	font_desc = get_system_monospace_font ();
	if (font_desc) {
            override_widget_font (dialog->forecast_text, font_desc);
            pango_font_description_free (font_desc);
	}

        buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (dialog->forecast_text));
        forecast = g_strdup (weather_info_get_forecast (dialog->applet->mateweather_info));
        if (forecast) {
            forecast = g_strstrip (replace_multiple_new_lines (forecast));
            gtk_text_buffer_set_text (buffer, forecast, -1);
            g_free (forecast);
        } else {
            gtk_text_buffer_set_text (buffer, _("Forecast not currently available for this location."), -1);
        }
        gtk_widget_show (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dialog->weather_notebook), 1));
    } else {
        gtk_widget_hide (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dialog->weather_notebook), 1));
    }

    /* Update radar map */
    if (dialog->applet->mateweather_pref.radar_enabled) {
        GdkPixbufAnimation *radar;

	radar = weather_info_get_radar (info);
        if (radar) {
            gtk_image_set_from_animation (GTK_IMAGE (dialog->radar_image), radar);
        }
        gtk_widget_show (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dialog->weather_notebook), 2));
        gtk_window_set_default_size (GTK_WINDOW (dialog), 570, 440);
    } else {
        gtk_widget_hide (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dialog->weather_notebook), 2));
        gtk_window_set_default_size (GTK_WINDOW (dialog), 590, 340);
    }
}

static void
mateweather_dialog_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
    MateWeatherDialog *dialog;

    dialog = MATEWEATHER_DIALOG (object);

    switch (prop_id) {
        case PROP_MATEWEATHER_APPLET:
            dialog->applet = g_value_get_pointer (value);
            break;
    }
}

static void
mateweather_dialog_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
    MateWeatherDialog *dialog;

    dialog = MATEWEATHER_DIALOG (object);

    switch (prop_id) {
	case PROP_MATEWEATHER_APPLET:
	    g_value_set_pointer (value, dialog->applet);
	    break;
    }
}

static void
mateweather_dialog_init (MateWeatherDialog *dialog)
{
    gtk_widget_init_template (GTK_WIDGET (dialog));
}

static GObject*
mateweather_dialog_constructor (GType                  type,
                                guint                  n_construct_params,
                                GObjectConstructParam *construct_params)
{
    GObject *object;
    MateWeatherDialog *self;

    object = G_OBJECT_CLASS (mateweather_dialog_parent_class)->
        constructor (type, n_construct_params, construct_params);
    self = MATEWEATHER_DIALOG (object);

    mateweather_dialog_update (self);

    return object;
}

GtkWidget*
mateweather_dialog_new (MateWeatherApplet *applet)
{
    return g_object_new(MATEWEATHER_TYPE_DIALOG,
                        "mateweather-applet", applet,
                        NULL);
}

static void
mateweather_dialog_class_init (MateWeatherDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->set_property = mateweather_dialog_set_property;
    object_class->get_property = mateweather_dialog_get_property;
    object_class->constructor = mateweather_dialog_constructor;

    /* This becomes an OBJECT property when MateWeatherApplet is redone */
    g_object_class_install_property (object_class,
                                     PROP_MATEWEATHER_APPLET,
                                     g_param_spec_pointer ("mateweather-applet",
                                                           "MateWeather Applet",
                                                           "The MateWeather Applet",
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 WEATHER_RESOURCE_PATH "mateweather-dialog.ui");

    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, weather_notebook);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_location);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_update);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_cond);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_sky);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_temp);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_dew);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_humidity);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_wind);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_pressure);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_vis);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_apparent);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_sunrise);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_sunset);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, cond_image);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, forecast_text);
    gtk_widget_class_bind_template_child (widget_class, MateWeatherDialog, radar_image);

    gtk_widget_class_bind_template_callback (widget_class, response_cb);
    gtk_widget_class_bind_template_callback (widget_class, link_cb);
}
