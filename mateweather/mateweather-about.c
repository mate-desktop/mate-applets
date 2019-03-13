/* $Id$ */

/*
 *  Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 *
 *  This code released under the GNU GPL.
 *  Read the file COPYING for more information.
 *
 *  About box
 *
 */

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE

#include "mateweather.h"
#include "mateweather-about.h"

void mateweather_about_run(MateWeatherApplet* gw_applet)
{
	static const gchar* authors[] = {
		"Todd Kulesza <fflewddur@dropline.net>",
		"Philip Langdale <philipl@mail.utexas.edu>",
		"Ryan Lortie <desrt@desrt.ca>",
		"Davyd Madeley <davyd@madeley.id.au>",
		"Spiros Papadimitriou <spapadim+@cs.cmu.edu>",
		"Kevin Vandersloot <kfv101@psu.edu>",
		NULL
	};

	const gchar* documenters[] = {
		"Dan Mueth <d-mueth@uchicago.edu>",
		"Spiros Papadimitriou <spapadim+@cs.cmu.edu>",
		N_("Sun GNOME Documentation Team <gdocteam@sun.com>"),
		"Davyd Madeley <davyd@madeley.id.au>",
		N_("MATE Documentation Team"),
		NULL
	};

#ifdef ENABLE_NLS
	const char **p;
	for (p = documenters; *p; ++p)
		*p = _(*p);
#endif

	gtk_show_about_dialog(NULL,
		"title", _("About Weather Report"),
		"version", VERSION,
		"copyright", _("Copyright \xC2\xA9 1999-2005 by S. Papadimitriou and others\n"
		               "Copyright \xc2\xa9 2012-2019 MATE developers"),
		"comments", _("A panel application for monitoring local weather conditions."),
		"authors", authors,
		"documenters", documenters,
		"translator-credits", _("translator-credits"),
		"logo-icon-name", "weather-storm",
		NULL);
}
