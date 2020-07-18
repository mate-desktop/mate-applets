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

#ifndef __BATTSTAT_PREFERENCES_H__
#define __BATTSTAT_PREFERENCES_H__

#include <glib.h>

#include "battstat.h"

G_BEGIN_DECLS

#define BATTSTAT_TYPE_PREFERENCES battstat_preferences_get_type ()
G_DECLARE_FINAL_TYPE (BattstatPreferences, battstat_preferences,
                      BATTSTAT, PREFERENCES, GtkDialog)

GtkWidget        *battstat_preferences_new              (ProgressData *battstat);

G_END_DECLS

#endif /* __BATTSTAT_PREFERENCES_H__ */
