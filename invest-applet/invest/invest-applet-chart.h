/*
 * MATE Invest Applet - Chart functionality header
 * Copyright (C) 2025 MATE developers
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

#ifndef INVEST_APPLET_CHART_H
#define INVEST_APPLET_CHART_H

#include <glib.h>
#include <gtk/gtk.h>
#include "invest-applet.h"
typedef struct _InvestChart InvestChart;

InvestChart* invest_chart_new (InvestApplet *applet);
void invest_chart_free (InvestChart *chart);
void invest_chart_show (InvestChart *chart);
void invest_chart_hide (InvestChart *chart);
gboolean invest_chart_is_visible (InvestChart *chart);
void invest_chart_refresh_data (InvestChart *chart);

#endif /* INVEST_APPLET_CHART_H */
