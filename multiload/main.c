/* MATE multiload panel applet
 * (C) 1997 The Free Software Foundation
 *
 * Authors: Tim P. Gerla
 *          Martin Baulig
 *          Todd Kulesza
 *
 * With code from wmload.c, v0.9.2, apparently by Ryan Land, rland@bc1.com.
 *
 */

#include <config.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

#include <glibtop.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-gsettings.h>

#include "global.h"

static void
about_cb (GtkAction       *action,
          MultiloadApplet *ma)
{
    const gchar * const authors[] =
    {
        "Martin Baulig <martin@home-of-linux.org>",
        "Todd Kulesza <fflewddur@dropline.net>",
        "Benoît Dejean <TazForEver@dlfp.org>",
        "Davyd Madeley <davyd@madeley.id.au>",
        NULL
    };

    const gchar* documenters[] =
    {
        "Chee Bin HOH <cbhoh@gnome.org>",
        N_("Sun GNOME Documentation Team <gdocteam@sun.com>"),
        N_("MATE Documentation Team"),
        NULL
    };

#ifdef ENABLE_NLS
    const char **p;
    for (p = documenters; *p; ++p)
        *p = _(*p);
#endif

    gtk_show_about_dialog (NULL,
    "title",                 _("About System Monitor"),
    "version",               VERSION,
    "copyright",             _("Copyright \xc2\xa9 1999-2005 Free Software Foundation and others\n"
                             "Copyright \xc2\xa9 2012-2020 MATE developers"),
    "comments",              _("A system load monitor capable of displaying graphs "
                               "for CPU, ram, and swap space use, plus network "
                               "traffic."),
    "authors",               authors,
    "documenters",           documenters,
    "translator-credits",    _("translator-credits"),
    "logo-icon-name",        "utilities-system-monitor",
    NULL);
}

static void
help_cb (GtkAction       *action,
         MultiloadApplet *ma)
{
    GError *error = NULL;

    gtk_show_uri_on_window (NULL,
                            "help:mate-multiload",
                            gtk_get_current_event_time (),
                            &error);

    if (error) { /* FIXME: the user needs to see this */
        g_warning ("help error: %s\n", error->message);
        g_error_free (error);
        error = NULL;
    }
}

/* run the full-scale system process monitor */

static void
start_procman (MultiloadApplet *ma)
{
    GError *error = NULL;
    GDesktopAppInfo *appinfo;
    gchar *monitor;
    GdkAppLaunchContext *launch_context;
    GdkDisplay *display;
    GAppInfo *app_info;
    GdkScreen *screen;

    g_return_if_fail (ma != NULL);

    monitor = g_settings_get_string (ma->settings, "system-monitor");
    if (monitor == NULL)
        monitor = g_strdup ("mate-system-monitor.desktop");

    screen = gtk_widget_get_screen (GTK_WIDGET (ma->applet));
    appinfo = g_desktop_app_info_new (monitor);
    if (appinfo) {
        GdkAppLaunchContext *context;
        display = gdk_screen_get_display (screen);
        context = gdk_display_get_app_launch_context (display);
        gdk_app_launch_context_set_screen (context, screen);
        g_app_info_launch (G_APP_INFO (appinfo), NULL, G_APP_LAUNCH_CONTEXT (context), &error);
        g_object_unref (context);
        g_object_unref (appinfo);
    }
    else {
        app_info = g_app_info_create_from_commandline ("mate-system-monitor",
                                                       _("Start system-monitor"),
                                                       G_APP_INFO_CREATE_NONE,
                                                       &error);

        if (!error) {
            display = gdk_screen_get_display (screen);
            launch_context = gdk_display_get_app_launch_context (display);
            gdk_app_launch_context_set_screen (launch_context, screen);
            g_app_info_launch (app_info, NULL, G_APP_LAUNCH_CONTEXT (launch_context), &error);

            g_object_unref (launch_context);
        }
    }
    g_free (monitor);

    if (error) {
        GtkWidget *dialog;

        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         _("There was an error executing '%s': %s"),
                                         "mate-system-monitor",
                                         error->message);

        g_signal_connect (dialog, "response",
                          G_CALLBACK (gtk_widget_destroy),
                          NULL);

        gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
        gtk_window_set_screen (GTK_WINDOW (dialog), screen);

        gtk_widget_show (dialog);

        g_error_free (error);
    }
}

static void
start_procman_cb (GtkAction       *action,
                  MultiloadApplet *ma)
{
    start_procman (ma);
}

static void
multiload_change_size_cb(MatePanelApplet *applet, gint size, gpointer data)
{
    MultiloadApplet *ma = (MultiloadApplet *)data;

    multiload_applet_refresh(ma);

    return;
}

static void
multiload_change_orient_cb(MatePanelApplet *applet, gint arg1, gpointer data)
{
    MultiloadApplet *ma = data;
    multiload_applet_refresh((MultiloadApplet *)data);
    gtk_widget_show (GTK_WIDGET (ma->applet));
    return;
}

static void
multiload_destroy_cb(GtkWidget *widget, gpointer data)
{
    gint i;
    MultiloadApplet *ma = data;

    for (i = 0; i < graph_n; i++)
    {
        load_graph_stop(ma->graphs[i]);
        if (ma->graphs[i]->colors)
        {
            g_free (ma->graphs[i]->colors);
            ma->graphs[i]->colors = NULL;
        }
        gtk_widget_destroy(ma->graphs[i]->main_widget);

        load_graph_unalloc(ma->graphs[i]);
        g_free(ma->graphs[i]);
    }

    netspeed_delete (ma->netspeed_in);
    netspeed_delete (ma->netspeed_out);

    if (ma->about_dialog)
        gtk_widget_destroy (ma->about_dialog);

    if (ma->prop_dialog)
        gtk_widget_destroy (ma->prop_dialog);

    gtk_widget_destroy(GTK_WIDGET(ma->applet));

    g_free (ma);

    return;
}


static gboolean
multiload_button_press_event_cb (GtkWidget *widget, GdkEventButton *event, MultiloadApplet *ma)
{
    g_return_val_if_fail (event != NULL, FALSE);
    g_return_val_if_fail (ma != NULL, FALSE);

    if (event->button == 1 && event->type == GDK_BUTTON_PRESS) {
        start_procman (ma);
        return TRUE;
    }
    return FALSE;
}

static gboolean
multiload_key_press_event_cb (GtkWidget *widget, GdkEventKey *event, MultiloadApplet *ma)
{
    g_return_val_if_fail (event != NULL, FALSE);
    g_return_val_if_fail (ma != NULL, FALSE);

    switch (event->keyval) {
    /* this list of keyvals taken from mixer applet, which seemed to have
       a good list of keys to use */
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
    case GDK_KEY_3270_Enter:
    case GDK_KEY_Return:
    case GDK_KEY_space:
    case GDK_KEY_KP_Space:
        /* activate */
        start_procman (ma);
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

/* update the tooltip to the graph's current "used" percentage */
void
multiload_applet_tooltip_update(LoadGraph *g)
{
    gchar *tooltip_text;
    MultiloadApplet *multiload;
    const char *tooltip_label [graph_n] = {
        [graph_cpuload]  = N_("Processor"),
        [graph_memload]  = N_("Memory"),
        [graph_netload2] = N_("Network"),
        [graph_swapload] = N_("Swap Space"),
        [graph_loadavg]  = N_("Load Average"),
        [graph_diskload] = N_("Disk")
    };
    const char *name;

    g_assert(g);

    multiload = g->multiload;

    /* label the tooltip intuitively */
    name = gettext (tooltip_label [g->id]);

    switch (g->id) {
        case graph_memload: {
            float user_percent, cache_percent;

            user_percent  = MIN ((float)(100 * multiload->memload_user)  / (float)(multiload->memload_total), 100.0f);
            cache_percent = MIN ((float)(100 * multiload->memload_cache) / (float)(multiload->memload_total), 100.0f);
            tooltip_text = g_strdup_printf (_("%s:\n"
                                              "%.01f%% in use by programs\n"
                                              "%.01f%% in use as cache"),
                                            name,
                                            user_percent,
                                            cache_percent);
            break;
        }
        case graph_loadavg: {
            tooltip_text = g_strdup_printf (_("The system load average is %0.02f"),
                                            multiload->loadavg1);
            break;
        }
        case graph_netload2: {
            char *tx_in, *tx_out;

            tx_in = netspeed_get(multiload->netspeed_in);
            tx_out = netspeed_get(multiload->netspeed_out);
            /* xgettext: same as in graphic tab of g-s-m */
            tooltip_text = g_strdup_printf(_("%s:\n"
                                           "Receiving %s\n"
                                           "Sending %s"),
                                           name, tx_in, tx_out);
            g_free(tx_in);
            g_free(tx_out);
            break;
        }
        default: {
            float ratio;
            float percent;

            if (g->id == graph_cpuload)
                ratio = multiload->cpu_used_ratio;
            else if (g->id == graph_swapload)
                ratio = multiload->swapload_used_ratio;
            else if (g->id == graph_diskload)
                ratio = multiload->diskload_used_ratio;
            else
                g_assert_not_reached ();

            percent = CLAMP (ratio * 100.0f, 0.0f, 100.0f);
            tooltip_text = g_strdup_printf(_("%s:\n"
                                           "%.01f%% in use"),
                                           name,
                                           percent);
        }
    }

    gtk_widget_set_tooltip_text(g->disp, tooltip_text);

    g_free(tooltip_text);
}

static void
multiload_create_graphs(MultiloadApplet *ma)
{
    struct { const char *label;
             const char *visibility_key;
             const char *name;
             int num_colours;
             LoadGraphDataFunc callback;
           } graph_types [graph_n] = {
             [graph_cpuload]  = { _("CPU Load"),     VIEW_CPULOAD_KEY,  "cpuload",  cpuload_n,  GetLoad },
             [graph_memload]  = { _("Memory Load"),  VIEW_MEMLOAD_KEY,  "memload",  memload_n,  GetMemory },
             [graph_netload2] = { _("Net Load"),     VIEW_NETLOAD_KEY,  "netload2", 6,          GetNet },
             [graph_swapload] = { _("Swap Load"),    VIEW_SWAPLOAD_KEY, "swapload", swapload_n, GetSwap },
             [graph_loadavg]  = { _("Load Average"), VIEW_LOADAVG_KEY,  "loadavg",  3,          GetLoadAvg },
             [graph_diskload] = { _("Disk Load"),    VIEW_DISKLOAD_KEY, "diskload", diskload_n, GetDiskLoad }
           };

    gint speed, size;
    guint net_threshold1;
    guint net_threshold2;
    guint net_threshold3;
    gint i;

    speed = g_settings_get_int (ma->settings, "speed");
    size = g_settings_get_int (ma->settings, "size");
    net_threshold1  = CLAMP (g_settings_get_uint (ma->settings, "netthreshold1"), MIN_NET_THRESHOLD1, MAX_NET_THRESHOLD1);
    net_threshold2  = CLAMP (g_settings_get_uint (ma->settings, "netthreshold2"), MIN_NET_THRESHOLD2, MAX_NET_THRESHOLD2);
    net_threshold3  = CLAMP (g_settings_get_uint (ma->settings, "netthreshold3"), MIN_NET_THRESHOLD3, MAX_NET_THRESHOLD3);
    if (net_threshold1 >= net_threshold2)
    {
       net_threshold1 = net_threshold2 - 1;
    }
    if (net_threshold2 >= net_threshold3)
    {
       net_threshold3 = net_threshold2 + 1;
    }
    speed = MAX (speed, 50);
    size = CLAMP (size, 10, 400);

    for (i = 0; i < graph_n; i++)
    {
        ma->graphs[i] = load_graph_new (ma,
                                        graph_types[i].num_colours,
                                        graph_types[i].label,
                                        i,
                                        speed,
                                        size,
                                        g_settings_get_boolean (ma->settings, graph_types[i].visibility_key),
                                        graph_types[i].name,
                                        graph_types[i].callback);
    }
    ma->nvme_diskstats = g_settings_get_boolean (ma->settings, "diskload-nvme-diskstats");
    /* for Network graph, colors[4] is grid line color, it should not be used in loop in load-graph.c */
    /* for Network graph, colors[5] is indicator color, it should not be used in loop in load-graph.c */
    ma->graphs[graph_netload2]->n = 4;
    ma->net_threshold1 = net_threshold1;
    ma->net_threshold2 = net_threshold2;
    ma->net_threshold3 = net_threshold3;
    ma->netspeed_in = netspeed_new (ma->graphs [graph_netload2]);
    ma->netspeed_out = netspeed_new (ma->graphs [graph_netload2]);
    /* for Load graph, colors[2] is grid line color, it should not be used in loop in load-graph.c */
    ma->graphs[graph_loadavg]->n = 2;
}

/* remove the old graphs and rebuild them */
void
multiload_applet_refresh(MultiloadApplet *ma)
{
    gint i;
    MatePanelAppletOrient orientation;

    /* stop and free the old graphs */
    for (i = 0; i < graph_n; i++)
    {
        if (!ma->graphs[i])
            continue;

        load_graph_stop(ma->graphs[i]);
        gtk_widget_destroy(ma->graphs[i]->main_widget);

        load_graph_unalloc(ma->graphs[i]);
        g_free(ma->graphs[i]);
    }

    if (ma->box)
        gtk_widget_destroy(ma->box);

    orientation = mate_panel_applet_get_orient(ma->applet);

    if ( (orientation == MATE_PANEL_APPLET_ORIENT_UP) ||
        (orientation == MATE_PANEL_APPLET_ORIENT_DOWN) ) {
        ma->box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    }
    else
        ma->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    gtk_container_add(GTK_CONTAINER(ma->applet), ma->box);

    /* create the N graphs, passing in their user-configurable properties with gsettings. */
    multiload_create_graphs (ma);

    /* only start and display the graphs the user has turned on */

    for (i = 0; i < graph_n; i++) {
        gtk_box_pack_start(GTK_BOX(ma->box),
                           ma->graphs[i]->main_widget,
                           TRUE, TRUE, 1);
        if (ma->graphs[i]->visible) {
            gtk_widget_show_all (ma->graphs[i]->main_widget);
        load_graph_start(ma->graphs[i]);
        }
    }
    gtk_widget_show (ma->box);

    return;
}

static const GtkActionEntry multiload_menu_actions [] = {
    { "MultiLoadProperties", "document-properties", N_("_Preferences"),
        NULL, NULL,
        G_CALLBACK (multiload_properties_cb) },
    { "MultiLoadRunProcman", "system-run", N_("_Open System Monitor"),
        NULL, NULL,
        G_CALLBACK (start_procman_cb) },
    { "MultiLoadHelp", "help-browser", N_("_Help"),
        NULL, NULL,
        G_CALLBACK (help_cb) },
    { "MultiLoadAbout", "help-about", N_("_About"),
        NULL, NULL,
        G_CALLBACK (about_cb) }
};

/* create a box and stuff the load graphs inside of it */
static gboolean
multiload_applet_new(MatePanelApplet *applet, const gchar *iid, gpointer data)
{
    GtkStyleContext *context;
    MultiloadApplet *ma;
    GSettings *lockdown_settings;
    GtkActionGroup *action_group;
    gchar *ui_path;

    context = gtk_widget_get_style_context (GTK_WIDGET (applet));
    gtk_style_context_add_class (context, "multiload-applet");

    ma = g_new0(MultiloadApplet, 1);

    ma->applet = applet;

    ma->about_dialog = NULL;
    ma->prop_dialog = NULL;
        ma->last_clicked = 0;

    g_set_application_name (_("System Monitor"));

    gtk_window_set_default_icon_name ("utilities-system-monitor");
    mate_panel_applet_set_background_widget (applet, GTK_WIDGET(applet));

    ma->settings = mate_panel_applet_settings_new (applet, "org.mate.panel.applet.multiload");
    mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);

    action_group = gtk_action_group_new ("Multiload Applet Actions");
    gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions (action_group,
                                  multiload_menu_actions,
                                  G_N_ELEMENTS (multiload_menu_actions),
                                  ma);
    ui_path = g_build_filename (MULTILOAD_MENU_UI_DIR, "multiload-applet-menu.xml", NULL);
    mate_panel_applet_setup_menu_from_file (applet, ui_path, action_group);
    g_free (ui_path);


    if (mate_panel_applet_get_locked_down (applet)) {
        GtkAction *action;

        action = gtk_action_group_get_action (action_group, "MultiLoadProperties");
        gtk_action_set_visible (action, FALSE);
    }

    lockdown_settings = g_settings_new ("org.mate.lockdown");
    if (g_settings_get_boolean (lockdown_settings, "disable-command-line") ||
        mate_panel_applet_get_locked_down (applet)) {
        GtkAction *action;

        /* When the panel is locked down or when the command line is inhibited,
           it seems very likely that running the procman would be at least harmful */
        action = gtk_action_group_get_action (action_group, "MultiLoadRunProcman");
        gtk_action_set_visible (action, FALSE);
    }
    g_object_unref (lockdown_settings);

    g_object_unref (action_group);

    g_signal_connect(G_OBJECT(applet), "change_size",
                     G_CALLBACK(multiload_change_size_cb), ma);
    g_signal_connect(G_OBJECT(applet), "change_orient",
                     G_CALLBACK(multiload_change_orient_cb), ma);
    g_signal_connect(G_OBJECT(applet), "destroy",
                     G_CALLBACK(multiload_destroy_cb), ma);
    g_signal_connect(G_OBJECT(applet), "button_press_event",
                     G_CALLBACK(multiload_button_press_event_cb), ma);
    g_signal_connect(G_OBJECT(applet), "key_press_event",
                     G_CALLBACK(multiload_key_press_event_cb), ma);

    multiload_applet_refresh (ma);

    gtk_widget_show(GTK_WIDGET(applet));

    return TRUE;
}

static gboolean
multiload_factory (MatePanelApplet *applet,
                   const gchar *iid,
                   gpointer data)
{
    gboolean retval = FALSE;

    glibtop_init();

    retval = multiload_applet_new(applet, iid, data);

    return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("MultiLoadAppletFactory",
                                       PANEL_TYPE_APPLET,
                                       "multiload",
                                       multiload_factory,
                                       NULL)
