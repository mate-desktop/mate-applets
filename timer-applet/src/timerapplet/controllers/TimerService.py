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

import dbus
import dbus.service
from timerapplet import core
from timerapplet import utils

DBUS_INTERFACE_NAMESPACE = 'net.launchpad.timerapplet.TimerApplet.Timer'

class TimerService(dbus.service.Object):
    def __init__(self, bus_name, object_path, timer):
        dbus.service.Object.__init__(self,
                                     dbus.service.BusName(bus_name, bus=dbus.SessionBus()),
                                     object_path)
        self._timer = timer
    
    @dbus.service.method(dbus_interface=DBUS_INTERFACE_NAMESPACE, in_signature='siii')
    def Start(self, name, hours, minutes, seconds):
        if self._timer.get_state() != core.Timer.STATE_IDLE:
            self._timer.reset()
        self._timer.set_duration(utils.hms_to_seconds(hours, minutes, seconds))
        self._timer.set_name(name)
        self._timer.start()
        
    @dbus.service.method(dbus_interface=DBUS_INTERFACE_NAMESPACE)
    def Stop(self):
        if self._timer.get_state() != core.Timer.STATE_IDLE:
            self._timer.reset()
    
    @dbus.service.method(dbus_interface=DBUS_INTERFACE_NAMESPACE)
    def PauseContinue(self):
        if self._timer.get_state() == core.Timer.STATE_RUNNING:
            self._timer.stop()
        elif self._timer.get_state() == core.Timer.STATE_PAUSED:
            self._timer.start()
