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

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#include "active-sensor-libnotify.h"

static void notif_closed_cb(NotifyNotification *notification,
                            ActiveSensor *active_sensor) {

    g_assert(notification != NULL && active_sensor != NULL);

    int i;

    g_debug("Notification was closed.. setting reference to NULL so we can show again if needed.");

    // set notif reference to NULL
    for (i = 0; i < NUM_NOTIFS; i++) {
        if (active_sensor->notification[i] == notification) {
            active_sensor->notification[i] = NULL;
            break;
        }
    }
}

void active_sensor_libnotify_notify_end(ActiveSensor *active_sensor,
                                        NotifType notif_type) {

    GError *error = NULL;
    if (active_sensor->notification[notif_type]) {
        g_debug("Closing notification");
        if(!notify_notification_close(active_sensor->notification[notif_type], &error)) {
            g_warning("Error closing notification: %s", error->message);
            g_error_free(error);
        }
        g_object_unref(active_sensor->notification[notif_type]);
        active_sensor->notification[notif_type] = NULL;
    }
}

void active_sensor_libnotify_notify(ActiveSensor *active_sensor,
                                    NotifType notif_type,
                                    const gchar *summary,
                                    const gchar *message,
                                    const gchar *icon_filename,
                                    gint timeout_msecs) {

    GError *error = NULL;

    if (!notify_is_initted()) {
        if (!notify_init(PACKAGE)) {
            return;
        }
    }
    g_debug("Doing notification %s: %s: %s", (notif_type == SENSOR_INTERFACE_ERROR ? "interface-error" : "other") ,summary, message);

    /* leave any existing notification since most likely hasn't changed */
    if (active_sensor->notification[notif_type] != NULL) {
        return;
        /* active_sensor_libnotify_notify_end(active_sensor, notif_type); */
    }

    /* now create a new one */
    g_debug("Creating new notification");
    active_sensor->notification[notif_type] = notify_notification_new(summary, message, icon_filename);
    g_signal_connect(active_sensor->notification[notif_type], "closed",
                     G_CALLBACK(notif_closed_cb),
                     active_sensor);

    notify_notification_set_urgency(active_sensor->notification[notif_type], NOTIFY_URGENCY_CRITICAL);

    /* timeout may have changed so update it */
    notify_notification_set_timeout(active_sensor->notification[notif_type], timeout_msecs);

    g_debug("showing notification");
    if (!notify_notification_show(active_sensor->notification[notif_type], &error)) {
        g_debug("Error showing notification: %s", error->message);
        g_error_free(error);
    }

}

