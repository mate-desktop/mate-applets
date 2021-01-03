/*
 * Copyright (C) 2020 MATE Development Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef NETSPEED_APPLET_H
#define NETSPEED_APPLET_H

#include <mate-panel-applet.h>

#include "backend.h"
#include "netspeed-preferences.h"

#define NETSPEED_TYPE_APPLET netspeed_applet_get_type ()
G_DECLARE_FINAL_TYPE (NetspeedApplet, netspeed_applet,
                      NETSPEED, APPLET, MatePanelApplet)

GSettings * netspeed_applet_get_settings (NetspeedApplet *netspeed);
const gchar * netspeed_applet_get_current_device_name (NetspeedApplet *netspeed);
void netspeed_applet_display_help (GtkWidget *dialog, const gchar *section);

#endif
