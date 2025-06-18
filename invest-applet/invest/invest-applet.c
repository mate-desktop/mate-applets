/*
 * MATE Invest Applet
 * Copyright (C) 2004-2005 Raphael Slinckx
 * Copyright (C) 2009-2010 Enrico Minack
 * Copyright (C) 2012-2025 MATE developers
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
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

typedef struct _InvestApplet InvestApplet;
typedef struct _InvestAppletClass InvestAppletClass;

static gchar* create_stock_tooltip (InvestApplet *applet);
static void free_stock_data (InvestApplet *applet);
static gint get_valid_stock_indices (InvestApplet *applet, gint *valid_indices);
static void update_applet_text (InvestApplet *applet, const gchar *message, gdouble change_percent);
static gboolean invest_applet_cycle_stocks (InvestApplet *applet);
static void display_stock_at_index (InvestApplet *applet, gint stock_index);
static void clear_timeout (guint *timeout_id);

#define INVEST_TYPE_APPLET (invest_applet_get_type ())
#define INVEST_APPLET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), INVEST_TYPE_APPLET, InvestApplet))

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

    /* for cycling throuhg multiple stocks */
    gint cycle_position;
    guint cycle_timeout_id;
};

struct _InvestAppletClass {
    MatePanelAppletClass parent_class;
};

G_DEFINE_TYPE (InvestApplet, invest_applet, PANEL_TYPE_APPLET);

#define DEFAULT_UPDATE_INTERVAL 15 /* 15 minutes */
#define DEFAULT_CYCLE_INTERVAL   5 /* 5 seconds */

static void
invest_applet_update_display (InvestApplet *applet)
{
    const gchar *direction_icon;

    if (applet->stock_summary) {
        if (applet->change_percent > 0) {
            direction_icon = "invest_up";
        } else if (applet->change_percent < 0) {
            direction_icon = "invest_down";
        } else {
            direction_icon = "invest_neutral";
        }
    } else {
        direction_icon = "invest_neutral";
    }

    gtk_label_set_text (GTK_LABEL (applet->label), applet->stock_summary);
    gtk_image_set_from_icon_name (GTK_IMAGE (applet->direction_icon), direction_icon, GTK_ICON_SIZE_MENU);
}

static void
on_stock_data_received (SoupSession *session,
                        SoupMessage *msg,
                        gpointer     user_data)
{
    gpointer *user_data_with_index = (gpointer *)user_data;
    InvestApplet *applet = INVEST_APPLET (user_data_with_index[0]);
    gint symbol_index = GPOINTER_TO_INT (user_data_with_index[1]);

    JsonParser *parser = NULL;
    JsonNode *root;
    JsonObject *root_obj;
    GError *error = NULL;

    if (msg->status_code != SOUP_STATUS_OK) {
        g_warning ("Failed to fetch stock data for symbol %d: %s", symbol_index, msg->reason_phrase);
        goto cleanup;
    }

    parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, msg->response_body->data,
                                     msg->response_body->length, &error)) {
        g_warning ("Failed to parse JSON for symbol %d: %s", symbol_index, error->message);
        g_error_free (error);
        goto cleanup;
    }

    root = json_parser_get_root (parser);
    root_obj = json_node_get_object (root);

    /* Parse data for single stock. The expected JSON from the Yahoo Finance API should look like this:
     * {
     *   "chart": {
     *     "result": [{
     *       "meta": {
     *         "currency": "USD",
     *         "symbol": "IBM",
     *         "exchangeName": "NYQ",
     *         "instrumentType": "EQUITY",
     *         "regularMarketPrice": 288.998,
     *         "longName": "International Business Machines Corporation",
     *         "shortName": "International Business Machines",
     *         "previousClose": 291.2,
     *         ...
     *       },
     *       "timestamp": [...],
     *       "indicators": {...}
     *     }],
     *     "error": null
     *   }
     * }
     */
    if (json_object_has_member (root_obj, "chart")) {
        JsonObject *chart = json_object_get_object_member (root_obj, "chart");
        if (json_object_has_member (chart, "result") && !json_object_get_null_member (chart, "result")) {
            JsonArray *results = json_object_get_array_member (chart, "result");
            if (results && json_array_get_length (results) > 0) {
                JsonObject *result = json_array_get_object_element (results, 0);
                if (result) {
                    JsonObject *meta = json_object_get_object_member (result, "meta");

                    if (meta) {
                        gdouble current_price = json_object_get_double_member (meta, "regularMarketPrice");
                        gdouble prev_close = json_object_get_double_member (meta, "previousClose");

                        if (current_price > 0 && prev_close > 0) {
                            gdouble change_percent = ((current_price - prev_close) / prev_close) * 100.0;

                            /* store individual stock data */
                            applet->stock_prices[symbol_index] = current_price;
                            applet->stock_changes[symbol_index] = change_percent;
                            applet->stock_valid[symbol_index] = TRUE;
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
    applet->pending_requests--;

    if (applet->pending_requests == 0) {
        if (applet->total_symbols > 0) {
            clear_timeout (&applet->cycle_timeout_id);

            applet->cycle_position = 0;

            gint valid_indices[applet->total_symbols];
            gint valid_count = get_valid_stock_indices (applet, valid_indices);

            if (valid_count > 0) {
                display_stock_at_index (applet, valid_indices[0]);

                /* start cycle timer if we have multiple stocks */
                if (!applet->cycle_timeout_id && valid_count > 1) {
                    applet->cycle_timeout_id = g_timeout_add_seconds (applet->cycle_interval, (GSourceFunc) invest_applet_cycle_stocks, applet);
                }

                gchar *tooltip_text = create_stock_tooltip (applet);
                gtk_widget_set_tooltip_text (GTK_WIDGET (applet), tooltip_text);
                g_free (tooltip_text);
            } else {
                update_applet_text (applet, _("No valid stock data"), 0.0);
            }
        } else {
            update_applet_text (applet, _("No valid stock data"), 0.0);
        }
    }

    g_free (user_data_with_index);
}

/* fetch stock data from Yahoo! Finance for all configured symbols */
static gboolean
invest_applet_update_stocks (gpointer user_data)
{
    InvestApplet *applet = INVEST_APPLET (user_data);
    gchar **symbols;
    gint symbol_count;

    if (!G_IS_SETTINGS (applet->settings)) {
        g_warning ("Settings not available yet");
        return G_SOURCE_CONTINUE;
    }

    symbols = g_settings_get_strv (applet->settings, "stock-symbols");

    if (!symbols || !symbols[0]) {
        update_applet_text (applet, _("No stocks configured"), 0.0);
        g_strfreev (symbols);
        return G_SOURCE_CONTINUE;
    }

    symbol_count = g_strv_length (symbols);

    free_stock_data (applet);

    applet->pending_requests = symbol_count;
    applet->total_symbols = symbol_count;
    applet->stock_symbols = g_strdupv (symbols);
    applet->stock_prices = g_malloc0 (symbol_count * sizeof (gdouble));
    applet->stock_changes = g_malloc0 (symbol_count * sizeof (gdouble));
    applet->stock_valid = g_malloc0 (symbol_count * sizeof (gboolean));

    applet->cycle_position = 0;
    clear_timeout (&applet->cycle_timeout_id);

    /* initialize networking */
    if (!applet->soup_session) {
        applet->soup_session = soup_session_new ();
    }

    /* make separate request for each stock symbol */
    for (gint i = 0; i < symbol_count; i++) {
        gchar *url = g_strdup_printf ("https://query2.finance.yahoo.com/v8/finance/chart/%s", symbols[i]);
        SoupMessage *msg = soup_message_new ("GET", url);

        /* HACK: avoid rate limiting */
        soup_message_headers_replace (msg->request_headers, "User-Agent",
                                      "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36");

        gpointer *user_data_with_index = g_malloc (2 * sizeof (gpointer));
        user_data_with_index[0] = applet;
        user_data_with_index[1] = GINT_TO_POINTER (i);

        soup_session_queue_message (applet->soup_session, msg, on_stock_data_received, user_data_with_index);
        g_free (url);
    }

    g_strfreev (symbols);
    return G_SOURCE_CONTINUE;
}

static void
invest_applet_preferences_cb (GtkAction    *action,
                              InvestApplet *applet)
{
    GtkWidget *dialog, *content_area, *entry, *label, *spin_button, *refresh_label, *cycle_label, *cycle_spin;
    GtkWidget *hbox1, *hbox2, *hbox3;
    gchar **symbols;
    gchar *symbols_text;
    gint response, refresh_interval, cycle_interval;

    dialog = gtk_dialog_new_with_buttons (_("Investment Applet Preferences"),
                                          NULL,
                                          GTK_DIALOG_MODAL,
                                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                                          _("_OK"), GTK_RESPONSE_OK,
                                          NULL);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 350, 150);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    gtk_container_set_border_width (GTK_CONTAINER (content_area), 12);

    /* Stock symbols */
    hbox1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    label = gtk_label_new (_("Stock symbols:"));
    gtk_label_set_xalign (GTK_LABEL (label), 0.0);
    gtk_widget_set_size_request (label, 120, -1);
    entry = gtk_entry_new ();
    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox1), entry, TRUE, TRUE, 0);

    /* Refresh interval */
    hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    refresh_label = gtk_label_new (_("Refresh interval (minutes):"));
    gtk_label_set_xalign (GTK_LABEL (refresh_label), 0.0);
    gtk_widget_set_size_request (refresh_label, 120, -1);
    spin_button = gtk_spin_button_new_with_range (1, 60, 1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spin_button), 0);
    gtk_box_pack_start (GTK_BOX (hbox2), refresh_label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox2), spin_button, FALSE, FALSE, 0);

    /* Cycle interval */
    hbox3 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    cycle_label = gtk_label_new (_("Cycle interval (seconds):"));
    gtk_label_set_xalign (GTK_LABEL (cycle_label), 0.0);
    gtk_widget_set_size_request (cycle_label, 120, -1);
    cycle_spin = gtk_spin_button_new_with_range (1, 60, 1);
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (cycle_spin), 0);
    gtk_box_pack_start (GTK_BOX (hbox3), cycle_label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox3), cycle_spin, FALSE, FALSE, 0);

    if (!G_IS_SETTINGS (applet->settings)) {
        g_warning ("Settings not available in preferences");
        gtk_widget_destroy (dialog);
        return;
    }

    symbols = g_settings_get_strv (applet->settings, "stock-symbols");
    symbols_text = g_strjoinv (",", symbols);
    gtk_entry_set_text (GTK_ENTRY (entry), symbols_text);
    g_free (symbols_text);
    g_strfreev (symbols);

    refresh_interval = g_settings_get_int (applet->settings, "refresh-interval");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_button), refresh_interval);

    cycle_interval = g_settings_get_int (applet->settings, "cycle-interval");
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (cycle_spin), cycle_interval);

    /* Pack all rows into content area */
    gtk_box_pack_start (GTK_BOX (content_area), hbox1, FALSE, FALSE, 6);
    gtk_box_pack_start (GTK_BOX (content_area), hbox2, FALSE, FALSE, 6);
    gtk_box_pack_start (GTK_BOX (content_area), hbox3, FALSE, FALSE, 6);

    gtk_widget_show_all (dialog);

    response = gtk_dialog_run (GTK_DIALOG (dialog));

    if (response == GTK_RESPONSE_OK) {
        const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
        gchar **new_symbols = g_strsplit (text, ",", -1);
        gint new_refresh_interval = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin_button));
        gint new_cycle_interval = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (cycle_spin));

        /* trim whitespaces */
        for (gint i = 0; new_symbols[i]; i++) {
            g_strstrip (new_symbols[i]);
        }

        g_settings_set_strv (applet->settings, "stock-symbols", (const gchar * const *)new_symbols);
        g_settings_set_int (applet->settings, "refresh-interval", new_refresh_interval);
        g_settings_set_int (applet->settings, "cycle-interval", new_cycle_interval);
        g_strfreev (new_symbols);

        /* Update refresh interval if changed */
        if (new_refresh_interval != applet->refresh_interval) {
            clear_timeout (&applet->update_timeout_id);
            applet->refresh_interval = new_refresh_interval;
            applet->update_timeout_id = g_timeout_add_seconds (new_refresh_interval * 60,
                                                               invest_applet_update_stocks,
                                                               applet);
        }

        /* Update cycle interval if changed */
        if (new_cycle_interval != applet->cycle_interval) {
            clear_timeout (&applet->cycle_timeout_id);
            applet->cycle_interval = new_cycle_interval;
            /* Restart cycle timer with new interval */
            if (applet->total_symbols > 0) {
                applet->cycle_timeout_id = g_timeout_add_seconds (new_cycle_interval, (GSourceFunc) invest_applet_cycle_stocks, applet);
            }
        }

        /* Trigger update */
        invest_applet_update_stocks (applet);
    }

    gtk_widget_destroy (dialog);
}

static void
invest_applet_refresh_cb (GtkAction    *action,
                          InvestApplet *applet)
{
    invest_applet_update_stocks (applet);
}

static void
invest_applet_help_cb (GtkAction    *action,
                       InvestApplet *applet)
{
    GError *error = NULL;

    gtk_show_uri_on_window (NULL, "help:mate-invest-applet", gtk_get_current_event_time (), &error);

    if (error) {
        GtkWidget *dialog = gtk_message_dialog_new (NULL,
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    _("Could not display help"));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                  "%s", error->message);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_error_free (error);
    }
}

static void
invest_applet_about_cb (GtkAction    *action,
                        InvestApplet *applet)
{
    const gchar *authors[] = {
        "Raphael Slinckx <raphael@slinckx.net>",
        "Enrico Minack <enrico-minack@gmx.de>",
        "MATE developers",
        NULL
    };

    gtk_show_about_dialog (NULL,
                           "program-name", _("Invest"),
                           "logo-icon-name", "mate-invest-applet",
                           "version", VERSION,
                           "comments", _("Track your invested money."),
                           "copyright", "Copyright \xc2\xa9 2004-2005 Raphael Slinckx\nCopyright \xc2\xa9 2009-2010 Enrico Minack\nCopyright \xc2\xa9 2012-2025 MATE developers",
                           "authors", authors,
                           NULL);
}

static gboolean
invest_applet_cycle_stocks (InvestApplet *applet)
{
    if (applet->total_symbols == 0) {
        return G_SOURCE_CONTINUE;
    }

    /* find valid stocks */
    gint valid_indices[applet->total_symbols];
    gint valid_count = get_valid_stock_indices (applet, valid_indices);

    if (valid_count == 0) {
        update_applet_text (applet, _("No valid stock data"), 0.0);
        return G_SOURCE_CONTINUE;
    }

    if (valid_count == 1) {
        /* single stock, nothing to cycle, stop timer */
        return G_SOURCE_REMOVE;
    }

    /* multiple stocks, cycle to next stock */
    applet->cycle_position = (applet->cycle_position + 1) % valid_count;

    /* display current stock */
    display_stock_at_index (applet, valid_indices[applet->cycle_position]);

    return G_SOURCE_CONTINUE;
}

static gboolean
invest_applet_button_press (GtkWidget      *widget,
                            GdkEventButton *event,
                            InvestApplet   *applet)
{
    if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
        /* cycle to next stock */
        invest_applet_cycle_stocks (applet);
        return TRUE;
    }

    return FALSE;
}

static void
invest_applet_destroy (GtkWidget    *widget,
                       InvestApplet *applet)
{
    clear_timeout (&applet->update_timeout_id);
    clear_timeout (&applet->cycle_timeout_id);

    if (applet->soup_session) {
        g_object_unref (applet->soup_session);
        applet->soup_session = NULL;
    }

    if (applet->settings) {
        g_object_unref (applet->settings);
        applet->settings = NULL;
    }

    free_stock_data (applet);
}

static void
invest_applet_init (InvestApplet *applet)
{
    GtkWidget *hbox;

    /* Settings will be initialized in the factory function */
    applet->settings = NULL;
    applet->refresh_interval = DEFAULT_UPDATE_INTERVAL;
    applet->cycle_interval = DEFAULT_CYCLE_INTERVAL;

    applet->pending_requests = 0;
    applet->total_symbols = 0;
    applet->stock_symbols = NULL;
    applet->stock_prices = NULL;
    applet->stock_changes = NULL;
    applet->stock_valid = NULL;

    /* init stock cycling in panel display */
    applet->cycle_position = 0;
    applet->cycle_timeout_id = 0;

    /* init networking */
    applet->soup_session = soup_session_new ();

    /* UI */
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

    applet->direction_icon = gtk_image_new_from_icon_name ("dialog-information", GTK_ICON_SIZE_MENU);
    applet->label = gtk_label_new (_("Loading..."));

    gtk_box_pack_start (GTK_BOX (hbox), applet->direction_icon, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), applet->label, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (applet), hbox);

    g_signal_connect (G_OBJECT (applet), "button-press-event",
                      G_CALLBACK (invest_applet_button_press), applet);
    g_signal_connect (G_OBJECT (applet), "destroy",
                      G_CALLBACK (invest_applet_destroy), applet);

    const GtkActionEntry menu_actions[] = {
        { "InvestRefresh", "view-refresh", N_("_Refresh"), NULL, NULL, G_CALLBACK (invest_applet_refresh_cb) },
        { "InvestPreferences", "preferences-system", N_("_Preferences"), NULL, NULL, G_CALLBACK (invest_applet_preferences_cb) },
        { "InvestHelp", "help-browser", N_("_Help"), NULL, NULL, G_CALLBACK (invest_applet_help_cb) },
        { "InvestAbout", "help-about", N_("_About"), NULL, NULL, G_CALLBACK (invest_applet_about_cb) }
    };

    GtkActionGroup *action_group = gtk_action_group_new ("Invest Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group, menu_actions, G_N_ELEMENTS (menu_actions), applet);

    const gchar *ui = "<menuitem name=\"Invest Refresh\" action=\"InvestRefresh\" />"
                      "<separator />"
                      "<menuitem name=\"Invest Preferences\" action=\"InvestPreferences\" />"
                      "<menuitem name=\"Invest Help\" action=\"InvestHelp\" />"
                      "<menuitem name=\"Invest About\" action=\"InvestAbout\" />";

    mate_panel_applet_setup_menu (MATE_PANEL_APPLET (applet), ui, action_group);
    g_object_unref (action_group);

    gtk_widget_show_all (GTK_WIDGET (applet));
}

static void
invest_applet_class_init (InvestAppletClass *class)
{
}

static gboolean
invest_applet_factory (MatePanelApplet *applet,
                       const gchar     *iid,
                       gpointer         data)
{
    if (!g_strcmp0 (iid, "InvestApplet")) {
        InvestApplet *invest_applet = INVEST_APPLET (applet);

#ifndef ENABLE_IN_PROCESS
        g_set_application_name (_("Investment Applet"));
#endif
        gtk_window_set_default_icon_name ("mate-invest-applet");

        /* Set applet flags first */
        mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);

        /* Initialize settings after applet is set up */
        invest_applet->settings = mate_panel_applet_settings_new (applet, "org.mate.panel.applet.invest");

        /* Load refresh interval from settings */
        if (G_IS_SETTINGS (invest_applet->settings)) {
            invest_applet->refresh_interval = g_settings_get_int (invest_applet->settings, "refresh-interval");
            invest_applet->cycle_interval = g_settings_get_int (invest_applet->settings, "cycle-interval");
        }

        /* Start periodic updates */
        invest_applet->update_timeout_id = g_timeout_add_seconds (invest_applet->refresh_interval * 60,
                                                                  invest_applet_update_stocks,
                                                                  invest_applet);

        /* Load initial data */
        invest_applet_update_stocks (invest_applet);
        invest_applet_update_display (invest_applet);

        return TRUE;
    }

    return FALSE;
}

static gchar*
create_stock_tooltip (InvestApplet *applet)
{
    GString *tooltip = g_string_new ("");
    gint valid_count = 0;

    g_string_append (tooltip, _("Portfolio Summary:\n"));

    for (gint i = 0; i < applet->total_symbols; i++) {
        if (applet->stock_valid[i]) {
            g_string_append_printf (tooltip, "%s: $%.2f (%.2f%%)\n",
                                    applet->stock_symbols[i],
                                    applet->stock_prices[i],
                                    applet->stock_changes[i]);
            valid_count++;
        } else {
            g_string_append_printf (tooltip, "%s: No data\n",
                                    applet->stock_symbols[i]);
        }
    }

    if (valid_count == 0) {
        g_string_free (tooltip, TRUE);
        return g_strdup (_("No valid stock data"));
    }

    return g_string_free (tooltip, FALSE);
}


static void
free_stock_data (InvestApplet *applet)
{
    g_strfreev (applet->stock_symbols);
    g_free (applet->stock_prices);
    g_free (applet->stock_changes);
    g_free (applet->stock_valid);
    g_free (applet->stock_summary);

    applet->stock_symbols = NULL;
    applet->stock_prices = NULL;
    applet->stock_changes = NULL;
    applet->stock_valid = NULL;
    applet->stock_summary = NULL;
}

static gint
get_valid_stock_indices (InvestApplet *applet, gint *valid_indices)
{
    gint valid_count = 0;

    for (gint i = 0; i < applet->total_symbols; i++) {
        if (applet->stock_valid[i]) {
            valid_indices[valid_count] = i;
            valid_count++;
        }
    }

    return valid_count;
}

static void
display_stock_at_index (InvestApplet *applet, gint stock_index)
{
    if (stock_index < 0 || stock_index >= applet->total_symbols || !applet->stock_symbols) {
        g_warning ("Invalid stock index %d (total: %d)", stock_index, applet->total_symbols);
        return;
    }

    // Remove the '=' symbol from the stock symbol (used for currency conversions)
    gchar *stock_symbol = g_strdup (applet->stock_symbols[stock_index]);
    if (strchr (applet->stock_symbols[stock_index], '=')) {
        g_free (stock_symbol);
        stock_symbol = g_strndup (applet->stock_symbols[stock_index], strchr (applet->stock_symbols[stock_index], '=') - applet->stock_symbols[stock_index]);
    }
    gchar *message = g_strdup_printf ("%s: $%.2f",
                                      stock_symbol,
                                      applet->stock_prices[stock_index]);
    update_applet_text (applet, message, applet->stock_changes[stock_index]);
    g_free (stock_symbol);
    g_free (message);
}

static void
clear_timeout (guint *timeout_id)
{
    if (*timeout_id) {
        g_source_remove (*timeout_id);
        *timeout_id = 0;
    }
}

static void
update_applet_text (InvestApplet *applet, const gchar *message, gdouble change_percent)
{
    g_free (applet->stock_summary);
    applet->stock_summary = g_strdup (message);
    applet->change_percent = change_percent;
    invest_applet_update_display (applet);
}

PANEL_APPLET_FACTORY ("InvestAppletFactory",
                      INVEST_TYPE_APPLET,
                      "Investment applet",
                      invest_applet_factory,
                      NULL)
