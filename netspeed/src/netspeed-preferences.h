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

#ifndef NETSPEED_PREFERENCES_H
#define NETSPEED_PREFERENCES_H

#include <gtk/gtk.h>

#include "netspeed.h"

#define NETSPEED_TYPE_PREFERENCES netspeed_preferences_get_type ()
G_DECLARE_FINAL_TYPE (NetspeedPreferences, netspeed_preferences,
                      NETSPEED, PREFERENCES, GtkDialog)

GtkWidget *netspeed_preferences_new (NetspeedApplet *netspeed);

#endif
