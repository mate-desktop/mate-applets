/*
 * MATE Invest Applet - Main applet header
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

#ifndef INVEST_APPLET_H
#define INVEST_APPLET_H

#include <glib.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS

typedef struct _InvestApplet InvestApplet;
typedef struct _InvestAppletClass InvestAppletClass;

struct _InvestApplet {
    MatePanelApplet parent;

    GtkWidget *label;
    GtkWidget *direction_icon; /* icon for stock price going up/down/neutral */
    GSettings *settings;
    SoupSession *soup_session;

    guint update_timeout_id;
    gchar *stock_summary;
    gdouble change_percent;
    gint refresh_interval;
    gint cycle_interval;

    gint pending_requests;
    gint total_symbols;
    gchar **stock_symbols;
    gdouble *stock_prices;
    gdouble *stock_changes;
    gboolean *stock_valid;

    /* for cycling through multiple stocks */
    gint cycle_position;
    guint cycle_timeout_id;

    /* Chart functionality */
    struct _InvestChart *chart;
};

struct _InvestAppletClass {
    MatePanelAppletClass parent_class;
};

#define INVEST_TYPE_APPLET (invest_applet_get_type())
#define INVEST_APPLET(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), INVEST_TYPE_APPLET, InvestApplet))
#define INVEST_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), INVEST_TYPE_APPLET, InvestAppletClass))
#define INVEST_IS_APPLET(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), INVEST_TYPE_APPLET))
#define INVEST_IS_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), INVEST_TYPE_APPLET))
#define INVEST_APPLET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), INVEST_TYPE_APPLET, InvestAppletClass))

GType invest_applet_get_type(void);

G_END_DECLS

#endif /* INVEST_APPLET_H */
