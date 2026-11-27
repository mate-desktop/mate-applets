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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "about-dialog.h"

void about_dialog_open(SensorsApplet *sensors_applet) {
    const gchar *authors[] = {
        "Alex Murray <murray.alex@gmail.com>",
        "Stefano Karapetsas <stefano@karapetsas.com>",
        NULL
    };

    /* Construct the about dialog */
    gtk_show_about_dialog(NULL,
                  "icon-name", "mate-sensors-applet",
                  "program-name", PACKAGE_NAME,
                  "version", PACKAGE_VERSION,
                  "copyright", _("Copyright \xc2\xa9 2005-2009 Alex Murray\n"
                                 "Copyright \xc2\xa9 2011 Stefano Karapetsas\n"
                                 "Copyright \xc2\xa9 2012-2021 MATE developers"),
                  "authors", authors,
                  "documenters", authors,
                  /* To translator: Put your name here to show up in the About dialog as the translator */
                  "translator-credits", _("translator-credits"),
                  "logo-icon-name", SENSORS_APPLET_ICON,
                  "website", PACKAGE_URL,
                  NULL);

}
