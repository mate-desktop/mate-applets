# Copyright (C) 2008 Jimmy Do <jimmydo@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import gobject
import pynotify

class Notifier(object):
    _NOTIFICATION_REDISPLAY_INTERVAL_SECONDS = 60

    def __init__(self, app_name, icon, attach):
        self._icon = icon
        self._attach = attach
        self._notify = None
        self._handler_id = None
        self._timeout_id = None
        
        if not pynotify.is_initted():
            pynotify.init(app_name)

    def begin(self, summary, body, get_reminder_message_func):
        # NOTE: This callback wrapper is to workaround an API-breaking change present in
        # the version of libmatenotify used by Fedora 10. The API break adds an additional
        # 'reason' parameter to the callback signature. This was fixed before
        # the latest official release of libmatenotify, but it looks like Fedora 10
        # is using an unofficial libmatenotify build that still contains the API-breaking change.
        def closed_callback_wrapper(notification, reason_UNUSED=None):
            self._on_notification_closed(notification, get_reminder_message_func)

        self.end()
        self._notify = pynotify.Notification(summary, body, self._icon)
        self._handler_id = self._notify.connect('closed', closed_callback_wrapper)
        self._notify.show()

    def end(self):
        if self._notify is not None:
            if self._timeout_id is not None:
                gobject.source_remove(self._timeout_id)
                self._timeout_id = None
            self._notify.disconnect(self._handler_id)
            self._handler_id = None
            try:
                self._notify.close()
            except gobject.GError:
                # Throws a GError exception if the notification bubble has already been closed.
                # Ignore the exception.
                pass
            self._notify = None

    def _on_notification_closed(self, notification, get_reminder_message_func):
        self._timeout_id = gobject.timeout_add(Notifier._NOTIFICATION_REDISPLAY_INTERVAL_SECONDS * 1000,
                                               self._on_notification_redisplay_timeout,
                                               get_reminder_message_func)
        
    def _on_notification_redisplay_timeout(self, get_reminder_message_func):
        message = get_reminder_message_func()
        self._notify.props.body = message
        self._notify.show()
        
        self._timeout_id = None
        return False
