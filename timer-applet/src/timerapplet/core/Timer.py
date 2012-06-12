# Copyright (C) 2008 Jimmy Do <jimmydo@users.sourceforge.net>
# Copyright (C) 2010 Kenny Meyer <knny.myer@gmail.com>
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

import datetime
import time
import gobject

class Timer(gobject.GObject):
    (STATE_IDLE, STATE_RUNNING, STATE_PAUSED, STATE_FINISHED) = xrange(4)
    
    __gsignals__ = {'time-changed':
                        (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, ()),
                    'state-changed':
                        (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, ())}
    
    def __init__(self):
        gobject.GObject.__init__(self)
        self._state = Timer.STATE_IDLE
        self._duration_seconds = 0
        self._remaining_seconds = 0
        self._end_time = 0
        self._name = ''
        self._command = ''
        self._next_timer = ''
        self._auto_start = False
    
    def set_duration(self, seconds):
        """Set the duration of the timer in seconds."""
        assert self._state == Timer.STATE_IDLE
        self._duration_seconds = seconds
        
    def get_duration(self):
        """Return the duration of the timer in seconds."""
        return self._duration_seconds
        
    def set_name(self, name):
        """Set the name of the timer."""
        assert self._state == Timer.STATE_IDLE
        self._name = name
    
    def get_name(self):
        """Return the name of the timer."""
        return self._name

    def set_command(self, command):
        """Set the command to run of the timer."""
        assert self._state == Timer.STATE_IDLE
        self._command = command
    
    def get_command(self):
        """Return the name of the command of the timer."""
        return self._command
    
    def set_next_timer(self, timer):
        """Set the next timeer of the timer."""
        assert self._state == Timer.STATE_IDLE
        self._next_timer = timer

    def get_next_timer(self):
        """Get the next timer of the timer."""
        return self._next_timer
    
    def set_auto_start(self, auto_start):
        """Set the auto-start value of the timer."""
        assert self._state == Timer.STATE_IDLE
        self._auto_start = auto_start

    def get_auto_start(self):
        """Get the auto-start value."""
        return self._auto_start

    def start(self):
        """Start or resume the timer.
        
        This method should only be called when the timer is IDLE or PAUSED.
        
        """
        assert self._state == Timer.STATE_IDLE or self._state == Timer.STATE_PAUSED
        self._timer_transition_to_state(Timer.STATE_RUNNING)

    def stop(self):
        """Pause the timer.
        
        This method should only be called when the timer is RUNNING.
        
        """
        assert self._state == Timer.STATE_RUNNING
        self._timer_transition_to_state(Timer.STATE_PAUSED)

    def reset(self):
        """Reset the timer.
        
        This method should only be called when the timer is not IDLE.
        
        """
        assert self._state != Timer.STATE_IDLE
        self._timer_transition_to_state(Timer.STATE_IDLE)
        
    def get_state(self):
        """Return the current state."""
        return self._state
        
    def get_remaining_time(self):
        """Return the remaining time in seconds."""
        return min(self._duration_seconds, max(0, self._remaining_seconds))
        
    def get_end_time(self):
        """Return a datetime object representing the end time.
        
        This method should only be called when the timer is RUNNING or FINISHED.
        
        """
        assert self._state == Timer.STATE_RUNNING or self._state == Timer.STATE_FINISHED
        return datetime.datetime.fromtimestamp(self._end_time)
    
    def _timer_set_state(self, state):
        self._state = state
        self.emit('state-changed')
        
    def _timer_transition_to_state(self, dest_state):
        cur_time = int(time.time())
        
        if dest_state == Timer.STATE_IDLE:
            self._end_time = 0
            self._set_remaining_time(self._duration_seconds)
        elif dest_state == Timer.STATE_RUNNING:
            assert self._duration_seconds >= 0
            
            if self._state == Timer.STATE_IDLE:
                self._end_time = cur_time + self._duration_seconds
                self._set_remaining_time(self._duration_seconds)
            elif self._state == Timer.STATE_PAUSED:
                self._end_time = cur_time + self._remaining_seconds
                
            gobject.timeout_add(500, self._on_timeout)
        elif dest_state == Timer.STATE_PAUSED:
            self._set_remaining_time(self._end_time - cur_time)
            self._end_time = 0
        elif dest_state == Timer.STATE_FINISHED:
            pass
        else:
            assert False
            
        self._timer_set_state(dest_state)

    def _on_timeout(self):
        if self._state != Timer.STATE_RUNNING:
            return False # remove timeout source
    
        new_remaining = self._end_time - int(time.time())
        if self._remaining_seconds != new_remaining:
            self._set_remaining_time(new_remaining)
        
        if self._remaining_seconds < 0:
            self._timer_transition_to_state(Timer.STATE_FINISHED)
            return False # remove timeout source
        return True # keep timeout source
        
    def _set_remaining_time(self, new_remaining):
        self._remaining_seconds = new_remaining
        self.emit('time-changed')
