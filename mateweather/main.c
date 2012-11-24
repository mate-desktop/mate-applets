/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  Main applet widget
 *
 */

#include <glib.h>
#include <config.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include <libmateweather/mateweather-prefs.h>

#include "mateweather.h"
#include "mateweather-pref.h"
#include "mateweather-dialog.h"
#include "mateweather-applet.h"


static gboolean mateweather_applet_new(MatePanelApplet* applet, const gchar* iid, gpointer data)
{
	MateWeatherApplet* gw_applet;

	gw_applet = g_new0(MateWeatherApplet, 1);

	gw_applet->applet = applet;
	gw_applet->mateweather_info = NULL;
	gw_applet->settings = mate_panel_applet_settings_new (applet, "org.mate.weather");

	mateweather_applet_create(gw_applet);

	mateweather_prefs_load(&gw_applet->mateweather_pref, gw_applet->settings);

	mateweather_update(gw_applet);

	return TRUE;
}

static gboolean mateweather_applet_factory(MatePanelApplet* applet, const gchar* iid, gpointer data)
{
	gboolean retval = FALSE;

	retval = mateweather_applet_new(applet, iid, data);

	return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY("MateWeatherAppletFactory", PANEL_TYPE_APPLET, "mateweather", mateweather_applet_factory, NULL)
