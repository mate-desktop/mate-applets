/*
 * MATE Invest Applet - Chart functionality
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

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <cairo/cairo.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>
#include "invest-applet-chart.h"

typedef struct _StockChartData StockChartData;

struct _StockChartData {
    gchar *symbol;
    gdouble *prices;
    gint64 *timestamps;
    gint data_count;
    gboolean valid;
};

struct _InvestChart {
    InvestApplet *applet;
    GtkWidget *window;
    GtkWidget *drawing_area;
    StockChartData *chart_data;
    gint chart_data_count;
    gchar *chart_range;
    gchar *chart_interval;
};

static void fetch_chart_data (InvestChart *chart);
static void on_chart_data_received (SoupSession *session, SoupMessage *msg, gpointer user_data);
static gboolean chart_draw_cb (GtkWidget *widget, cairo_t *cr, InvestChart *chart);
static gboolean chart_window_key_press (GtkWidget *widget, GdkEventKey *event, InvestChart *chart);
static void chart_range_button_clicked (GtkWidget *widget, InvestChart *chart);
static void create_chart_toolbar (InvestChart *chart, GtkWidget *parent);
static void free_chart_data (InvestChart *chart);
static void draw_loading_message (cairo_t *cr, gint width, gint height, const gchar *message);
static void on_chart_window_destroy (GtkWidget *widget, InvestChart *chart);

InvestChart*
invest_chart_new (InvestApplet *applet)
{
    InvestChart *chart = g_malloc0 (sizeof (InvestChart));
    chart->applet = applet;
    chart->chart_data = NULL;
    chart->chart_data_count = 0;
    chart->chart_range = g_strdup ("1d");
    chart->chart_interval = g_strdup ("1m");
    return chart;
}

void
invest_chart_free (InvestChart *chart)
{
    if (!chart) {
        return;
    }

    if (chart->window) {
        invest_chart_hide (chart);
    }

    free_chart_data (chart);
    g_free (chart->chart_range);
    g_free (chart->chart_interval);
    g_free (chart);
}

static void
on_chart_window_destroy (GtkWidget *widget, InvestChart *chart)
{
    chart->window = NULL;
    chart->drawing_area = NULL;
}

void
invest_chart_show (InvestChart *chart)
{
    if (chart->window && gtk_widget_get_visible (chart->window)) {
        /* Chart is already visible, just bring it to front */
        gtk_window_present (GTK_WINDOW (chart->window));
        return;
    }

    /* Set default chart range if not already set */
    if (!chart->chart_range) {
        chart->chart_range = g_strdup ("1d");
        chart->chart_interval = g_strdup ("1m");
    }

    /* Create chart window */
    chart->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (chart->window), _("Stock Chart"));
    gtk_window_set_default_size (GTK_WINDOW (chart->window), 800, 500);
    gtk_window_set_resizable (GTK_WINDOW (chart->window), TRUE);
    gtk_window_set_modal (GTK_WINDOW (chart->window), FALSE);

    g_signal_connect (chart->window, "destroy", G_CALLBACK (on_chart_window_destroy), chart);

    /* Create main vertical box */
    GtkWidget *main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add (GTK_CONTAINER (chart->window), main_vbox);

    /* Create toolbar with time range buttons */
    create_chart_toolbar (chart, main_vbox);

    /* Create drawing area for chart */
    chart->drawing_area = gtk_drawing_area_new ();
    gtk_widget_set_vexpand (chart->drawing_area, TRUE);
    gtk_widget_set_hexpand (chart->drawing_area, TRUE);
    g_signal_connect (chart->drawing_area, "draw", G_CALLBACK (chart_draw_cb), chart);

    /* Add drawing area to main box */
    gtk_box_pack_start (GTK_BOX (main_vbox), chart->drawing_area, TRUE, TRUE, 0);

    /* We want to close window easily with Esc */
    g_signal_connect (chart->window, "key-press-event", G_CALLBACK (chart_window_key_press), chart);

    gtk_widget_set_can_focus (chart->window, TRUE);
    gtk_widget_show_all (chart->window);

    fetch_chart_data (chart);
}

void
invest_chart_hide (InvestChart *chart)
{
    if (!chart->window) {
        return;
    }

    gtk_widget_destroy (chart->window);
    chart->window = NULL;
    chart->drawing_area = NULL;
}

gboolean
invest_chart_is_visible (InvestChart *chart)
{
    if (!chart || !chart->window) {
        return FALSE;
    }
    return gtk_widget_get_visible (chart->window);
}

void
invest_chart_refresh_data (InvestChart *chart)
{
    /* Only refresh if chart is visible */
    if (chart && chart->window && gtk_widget_get_visible (chart->window)) {
        fetch_chart_data (chart);
    }
}

static void
fetch_chart_data (InvestChart *chart)
{
    gchar **symbols;
    gint symbol_count;

    if (!G_IS_SETTINGS (chart->applet->settings)) {
        g_warning ("Settings not available for chart data");
        return;
    }

    symbols = g_settings_get_strv (chart->applet->settings, "stock-symbols");
    if (!symbols || !symbols[0]) {
        g_strfreev (symbols);
        return;
    }

    symbol_count = g_strv_length (symbols);

    /* Free existing chart data */
    free_chart_data (chart);

    /* Allocate chart data array */
    chart->chart_data = g_malloc0 (symbol_count * sizeof (StockChartData));
    chart->chart_data_count = symbol_count;

    /* Initialize chart data for each stock symbol */
    for (gint i = 0; i < symbol_count; i++) {
        chart->chart_data[i].symbol = g_strdup (symbols[i]);
        chart->chart_data[i].valid = FALSE;
        chart->chart_data[i].prices = NULL;
        chart->chart_data[i].timestamps = NULL;
        chart->chart_data[i].data_count = 0;
    }

    /* This will show loading state */
    if (chart->window && gtk_widget_get_visible (chart->window)) {
        gtk_widget_queue_draw (chart->window);
    }

    /* Fetch chart the actual stock data */
    for (gint i = 0; i < symbol_count; i++) {
        const gchar *range = chart->chart_range ? chart->chart_range : "1d";
        const gchar *interval = chart->chart_interval ? chart->chart_interval : "1m";
        gchar *url = g_strdup_printf ("https://query2.finance.yahoo.com/v8/finance/chart/%s?interval=%s&range=%s", symbols[i], interval, range);
        SoupMessage *msg = soup_message_new ("GET", url);

        /* HACK: avoid rate limiting */
        soup_message_headers_replace (msg->request_headers, "User-Agent",
                                      "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");

        gpointer *user_data_with_index = g_malloc (2 * sizeof (gpointer));
        user_data_with_index[0] = chart;
        user_data_with_index[1] = GINT_TO_POINTER (i);

        /* Queue the actual request to the Yahoo Finance API */
        soup_session_queue_message (chart->applet->soup_session, msg, on_chart_data_received, user_data_with_index);
        g_free (url);
    }

    g_strfreev (symbols);
}

static void
on_chart_data_received (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
    gpointer *user_data_with_index = (gpointer *)user_data;
    InvestChart *chart = (InvestChart *)user_data_with_index[0];
    gint symbol_index = GPOINTER_TO_INT (user_data_with_index[1]);

    JsonParser *parser = NULL;
    JsonNode *root;
    JsonObject *root_obj;
    GError *error = NULL;

    if (msg->status_code != SOUP_STATUS_OK) {
        g_warning ("Failed to fetch chart data for symbol %d: %s", symbol_index, msg->reason_phrase);
        goto cleanup;
    }

    parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, msg->response_body->data,
                                     msg->response_body->length, &error)) {
        g_warning ("Failed to parse chart JSON for symbol %d: %s", symbol_index, error->message);
        g_error_free (error);
        goto cleanup;
    }

    root = json_parser_get_root (parser);
    root_obj = json_node_get_object (root);

    /* This was... fun. The structure for the chart data looks like this:
     *
     * {
     *   "chart": {
     *     "result": [
     *       {
     *         "timestamp": [ ... ],
     *         "indicators": {
     *           "quote": [{
     *             "close": [ ... ]
     *           }]
     *         }
     *       }
     *     ]
     *   }
     * }
     *
     * The closing prices for each stock map to the same index of
     * the corresponding timestamps, so we need to parse them all
     * separately but store them together for display later.
     */
    if (json_object_has_member (root_obj, "chart")) {
        JsonObject *chart_obj = json_object_get_object_member (root_obj, "chart");
        if (json_object_has_member (chart_obj, "result") && !json_object_get_null_member (chart_obj, "result")) {
            JsonArray *results = json_object_get_array_member (chart_obj, "result");
            if (results && json_array_get_length (results) > 0) {
                JsonObject *result = json_array_get_object_element (results, 0);
                if (result) {
                    /* Parse timestamps */
                    if (json_object_has_member (result, "timestamp")) {
                        JsonArray *timestamps = json_object_get_array_member (result, "timestamp");
                        if (timestamps) {
                            gint timestamp_count = json_array_get_length (timestamps);
                            /* Allocate memory for all the timestamps */
                            chart->chart_data[symbol_index].timestamps = g_malloc0 (timestamp_count * sizeof (gint64));
                            chart->chart_data[symbol_index].data_count = timestamp_count;

                            /* Then store them */
                            for (gint i = 0; i < timestamp_count; i++) {
                                JsonNode *timestamp_node = json_array_get_element (timestamps, i);
                                if (json_node_get_value_type (timestamp_node) == G_TYPE_INT64) {
                                    chart->chart_data[symbol_index].timestamps[i] = json_node_get_int (timestamp_node);
                                }
                            }
                        }
                    }

                    /* Parse price data */
                    if (json_object_has_member (result, "indicators")) {
                        JsonObject *indicators = json_object_get_object_member (result, "indicators");
                        if (json_object_has_member (indicators, "quote")) {
                            JsonArray *quotes = json_object_get_array_member (indicators, "quote");
                            if (quotes && json_array_get_length (quotes) > 0) {
                                /* For some reason quote is an array, but there's only one quote for each stock symbol */
                                JsonObject *quote = json_array_get_object_element (quotes, 0);
                                if (quote && json_object_has_member (quote, "close")) {
                                    JsonArray *close_prices = json_object_get_array_member (quote, "close");
                                    if (close_prices) {
                                        gint price_count = json_array_get_length (close_prices);
                                        /* Allocate memory for all the closing prices */
                                        chart->chart_data[symbol_index].prices = g_malloc0 (price_count * sizeof (gdouble));

                                        /* Then store them */
                                        for (gint i = 0; i < price_count; i++) {
                                            JsonNode *price_node = json_array_get_element (close_prices, i);
                                            if (json_node_get_value_type (price_node) == G_TYPE_DOUBLE) {
                                                chart->chart_data[symbol_index].prices[i] = json_node_get_double (price_node);
                                            }
                                        }
                                        /* We assume that the existence of data constitutes valid data */
                                        chart->chart_data[symbol_index].valid = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

cleanup:
    if (parser) {
        g_object_unref (parser);
    }

    /* Redraw chart if window is still visible */
    if (chart->window && gtk_widget_get_visible (chart->window)) {
        gtk_widget_queue_draw (chart->window);
    }

    g_free (user_data_with_index);
}

static gboolean
chart_draw_cb (GtkWidget *widget, cairo_t *cr, InvestChart *chart)
{
    gint width, height;
    GtkAllocation allocation;
    gtk_widget_get_size_request (widget, &width, &height);
    if (width <= 0 || height <= 0) {
        gtk_widget_get_allocated_size (widget, &allocation, NULL);
        width = allocation.width;
        height = allocation.height;
    }

    /* Clear background */
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_paint (cr);

    /* Check if we have any valid data to display */
    gboolean has_valid_data = FALSE;
    if (chart->chart_data && chart->chart_data_count > 0) {
        for (gint i = 0; i < chart->chart_data_count; i++) {
            if (chart->chart_data[i].valid && chart->chart_data[i].data_count > 0) {
                has_valid_data = TRUE;
                break;
            }
        }
    }

    if (!has_valid_data) {
        draw_loading_message (cr, width, height, _("Loading chart data..."));
        return FALSE;
    }

    /* Find min/max prices for scaling the chart properly */
    gdouble min_price = G_MAXDOUBLE;
    gdouble max_price = G_MINDOUBLE;
    gint64 min_time = G_MAXINT64;
    gint64 max_time = G_MININT64;

    for (gint i = 0; i < chart->chart_data_count; i++) {
        if (chart->chart_data[i].valid && chart->chart_data[i].data_count > 0) {
            for (gint j = 0; j < chart->chart_data[i].data_count; j++) {
                if (chart->chart_data[i].prices[j] > 0) {
                    min_price = MIN (min_price, chart->chart_data[i].prices[j]);
                    max_price = MAX (max_price, chart->chart_data[i].prices[j]);
                }
            }
            if (chart->chart_data[i].timestamps[0] > 0 &&
                chart->chart_data[i].timestamps[chart->chart_data[i].data_count - 1] > 0 &&
                chart->chart_data[i].timestamps[0] < 9999999999) { /* HACK: This works, but there's probably a better way */
                min_time = MIN (min_time, chart->chart_data[i].timestamps[0]);
                max_time = MAX (max_time, chart->chart_data[i].timestamps[chart->chart_data[i].data_count - 1]);
            }
        }
    }

    if (min_price == G_MAXDOUBLE || max_price == G_MINDOUBLE || min_time == G_MAXINT64 || max_time == G_MININT64) {
        draw_loading_message (cr, width, height, _("No chart data available"));
        return FALSE;
    }

    /* Add some padding to price range */
    gdouble price_range = max_price - min_price;
    min_price -= price_range * 0.05;
    max_price += price_range * 0.05;

    /* Calculate scaling factors */
    gdouble x_scale = (gdouble)(width - 100) / (max_time - min_time);
    gdouble y_scale = (gdouble)(height - 100) / (max_price - min_price);

    /* Draw grid */
    cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
    cairo_set_line_width (cr, 1.0);

    /* Horizontal grid lines */
    for (gint i = 0; i <= 10; i++) {
        gdouble y = 50 + (gdouble)i * (height - 100) / 10;
        cairo_move_to (cr, 50, y);
        cairo_line_to (cr, width - 50, y);
        cairo_stroke (cr);
    }

    /* Vertical grid lines */
    for (gint i = 0; i <= 10; i++) {
        gdouble x = 50 + (gdouble)i * (width - 100) / 10;
        cairo_move_to (cr, x, 50);
        cairo_line_to (cr, x, height - 50);
        cairo_stroke (cr);
    }

    /* Draw price labels */
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 10);

    for (gint i = 0; i <= 5; i++) {
        gdouble price = max_price - (gdouble)i * (max_price - min_price) / 5; /* Inverted for correct Y-axis */
        gdouble y = 50 + (gdouble)i * (height - 100) / 5;
        gchar price_str[32];
        g_snprintf (price_str, sizeof (price_str), "%.2f", price);
        cairo_move_to (cr, 5, y + 3);
        cairo_show_text (cr, price_str);
    }

    /* Draw time labels */
    for (gint i = 0; i <= 5; i++) {
        gint64 time = min_time + (gint64)i * (max_time - min_time) / 5;
        gdouble x = 50 + (gdouble)i * (width - 100) / 5;

        /* Convert timestamp to local time */
        GDateTime *dt = g_date_time_new_from_unix_local (time);
        gchar time_str[64];

        /* Format based on time range */
        if (chart->chart_range && g_strcmp0 (chart->chart_range, "1d") == 0) {
            /* For today view, show time */
            g_snprintf (time_str, sizeof (time_str), "%02d:%02d",
                        g_date_time_get_hour (dt), g_date_time_get_minute (dt));
        } else if (chart->chart_range && (g_strcmp0 (chart->chart_range, "5d") == 0 ||
                                          g_strcmp0 (chart->chart_range, "1mo") == 0 ||
                                          g_strcmp0 (chart->chart_range, "3mo") == 0)) {
            /* For weekly/monthly/quarterly views, show date */
            g_snprintf (time_str, sizeof (time_str), "%02d/%02d",
                        g_date_time_get_month (dt), g_date_time_get_day_of_month (dt));
        } else {
            /* For yearly and longer views, show month/year */
            g_snprintf (time_str, sizeof (time_str), "%02d/%d",
                        g_date_time_get_month (dt), g_date_time_get_year (dt));
        }

        cairo_move_to (cr, x - 20, height - 20);
        cairo_show_text (cr, time_str);
        g_date_time_unref (dt);
    }

    /* Draw stock price lines */
    const gchar *colors[] = {
        "#CC0000",
        "#3465A4",
        "#73D216",
        "#FCE94F",
        "#AD7FA8",
        "#F57900",
        "#C17D11",
        "#555753"
    };

    /* Collect valid stocks and their current prices for sorting */
    typedef struct {
        gint index;
        gdouble current_price;
    } StockSortInfo;

    StockSortInfo *sort_info = g_malloc0 (chart->chart_data_count * sizeof (StockSortInfo));
    gint valid_count = 0;

    for (gint i = 0; i < chart->chart_data_count; i++) {
        if (chart->chart_data[i].valid && chart->chart_data[i].data_count > 0) {
            sort_info[valid_count].index = i;
            sort_info[valid_count].current_price = chart->chart_data[i].prices[chart->chart_data[i].data_count - 1];
            valid_count++;
        }
    }

    /* Sort by current price (highest to lowest) */
    for (gint i = 0; i < valid_count - 1; i++) {
        for (gint j = i + 1; j < valid_count; j++) {
            if (sort_info[i].current_price < sort_info[j].current_price) {
                StockSortInfo temp = sort_info[i];
                sort_info[i] = sort_info[j];
                sort_info[j] = temp;
            }
        }
    }

    /* Draw lines and legend in sorted order */
    for (gint i = 0; i < valid_count; i++) {
        gint stock_index = sort_info[i].index;

        /* Set color for this stock */
        gint color_idx = i % G_N_ELEMENTS (colors);
        GdkRGBA color;
        gdk_rgba_parse (&color, colors[color_idx]);
        gdk_cairo_set_source_rgba (cr, &color);
        cairo_set_line_width (cr, 2.0);

        /* Draw price line */
        gboolean first_point = TRUE;

        for (gint j = 0; j < chart->chart_data[stock_index].data_count; j++) {
            if (chart->chart_data[stock_index].prices[j] > 0 &&
                chart->chart_data[stock_index].timestamps[j] > 0 &&
                chart->chart_data[stock_index].timestamps[j] < 9999999999) { /* Just to be safe... */

                gdouble x = 50 + (gdouble)j * (width - 100) / (chart->chart_data[stock_index].data_count - 1);
                gdouble y = 50 + (max_price - chart->chart_data[stock_index].prices[j]) * y_scale;

                if (first_point) {
                    cairo_move_to (cr, x, y);
                    first_point = FALSE;
                } else {
                    cairo_line_to (cr, x, y);
                }
            }
        }
        cairo_stroke (cr);

        /* Draw legend with current price */
        gdouble current_price = chart->chart_data[stock_index].prices[chart->chart_data[stock_index].data_count - 1];
        gchar *legend_text = g_strdup_printf ("%s: $%.2f", chart->chart_data[stock_index].symbol, current_price);
        cairo_move_to (cr, width - 200, 30 + i * 20);
        cairo_show_text (cr, legend_text);
        g_free (legend_text);
    }

    g_free (sort_info);

    return FALSE;
}

static gboolean
chart_window_key_press (GtkWidget *widget, GdkEventKey *event, InvestChart *chart)
{
    if (event->keyval == GDK_KEY_Escape) {
        invest_chart_hide (chart);
        return TRUE;
    }
    return FALSE;
}

static void
create_chart_toolbar (InvestChart *chart, GtkWidget *parent)
{
    /* Let's use a horizontal box as a toolbar */
    GtkWidget *toolbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);
    gtk_box_pack_start (GTK_BOX (parent), toolbar, FALSE, FALSE, 0);

    GtkWidget *label = gtk_label_new (_("Time Range:"));
    gtk_box_pack_start (GTK_BOX (toolbar), label, FALSE, FALSE, 5);

    /* Buttons for different time ranges */
    struct {
        const gchar *text;
        const gchar *range;
        const gchar *interval;
    } ranges[] = {
        { "Today", "1d", "1m" },
        { "Week", "5d", "5m" },
        { "Month", "1mo", "30m" },
        { "YTD", "ytd", "1d" },
        { "Year", "1y", "1d" },
        { "5Y", "5y", "1wk" },
        { "All", "max", "1mo" }
    };

    for (gint i = 0; i < G_N_ELEMENTS (ranges); i++) {
        GtkWidget *button = gtk_button_new_with_label (ranges[i].text);
        gtk_widget_set_size_request (button, 60, 30);

        /* Store range and interval data in button */
        g_object_set_data (G_OBJECT (button), "range", (gpointer)ranges[i].range);
        g_object_set_data (G_OBJECT (button), "interval", (gpointer)ranges[i].interval);

        g_signal_connect (button, "clicked", G_CALLBACK (chart_range_button_clicked), chart);
        gtk_box_pack_start (GTK_BOX (toolbar), button, FALSE, FALSE, 2);

        /* Disable the currently-selected button */
        if (chart->chart_range && g_strcmp0 (chart->chart_range, ranges[i].range) == 0) {
            gtk_widget_set_sensitive (button, FALSE);
        }
    }

    gtk_box_pack_start (GTK_BOX (toolbar), gtk_label_new (""), TRUE, TRUE, 0);
}

static void
free_chart_data (InvestChart *chart)
{
    if (!chart->chart_data) {
        return;
    }

    for (gint i = 0; i < chart->chart_data_count; i++) {
        g_free (chart->chart_data[i].symbol);
        g_free (chart->chart_data[i].prices);
        g_free (chart->chart_data[i].timestamps);
    }

    g_free (chart->chart_data);
    chart->chart_data = NULL;
    chart->chart_data_count = 0;
}

static void
chart_range_button_clicked (GtkWidget *widget, InvestChart *chart)
{
    /* Get range and interval from button data */
    const gchar *range = (const gchar *)g_object_get_data (G_OBJECT (widget), "range");
    const gchar *interval = (const gchar *)g_object_get_data (G_OBJECT (widget), "interval");

    if (!range || !interval) {
        return;
    }

    /* Update chart's range and interval */
    g_free (chart->chart_range);
    g_free (chart->chart_interval);
    chart->chart_range = g_strdup (range);
    chart->chart_interval = g_strdup (interval);

    /* Re-enable all buttons and disable the newly-selected one */
    GtkWidget *parent = gtk_widget_get_parent (widget);
    if (GTK_IS_BOX (parent)) {
        GList *children = gtk_container_get_children (GTK_CONTAINER (parent));
        for (GList *l = children; l; l = l->next) {
            GtkWidget *child = GTK_WIDGET (l->data);
            if (GTK_IS_BUTTON (child)) {
                gtk_widget_set_sensitive (child, TRUE);
            }
        }
        g_list_free (children);
    }

    /* Disable the clicked button */
    gtk_widget_set_sensitive (widget, FALSE);

    /* Fetch new chart data with updated range */
    fetch_chart_data (chart);
}

static void
draw_loading_message (cairo_t *cr, gint width, gint height, const gchar *message)
{
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
    cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 16);
    cairo_move_to (cr, width / 2 - 50, height / 2);
    cairo_show_text (cr, message);
}
