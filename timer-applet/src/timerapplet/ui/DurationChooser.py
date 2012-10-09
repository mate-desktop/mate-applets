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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

from gettext import gettext as _
import math
import gobject
import gtk

class DurationChooser(gtk.VBox):
    __gsignals__ = {'duration-changed': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, ())}
    MAX_HOURS = 48
    MAX_MINUTES = 999
    MAX_SECONDS = 999

    def __init__(self, size_group):
        gtk.VBox.__init__(self, False, 6)
        
        self._hours_spin = self._add_row(_('_Hours:'),
                                         size_group,
                                         DurationChooser.MAX_HOURS)
        self._minutes_spin = self._add_row(_('_Minutes:'),
                                           size_group,
                                           DurationChooser.MAX_MINUTES)
        self._seconds_spin = self._add_row(_('_Seconds:'),
                                           size_group,
                                           DurationChooser.MAX_SECONDS)
    
    def get_duration(self):
        """Return numerical representations of the values in the spinbuttons.
        
        This method does not just call get_value_as_int() on each spinbutton because
        get_value_as_int() does not return the most up-to-date value if the user is
        in the middle of editing the value in a spinbutton. Instead, it actually
        returns the value previously "confirmed" for the spinbutton (for example,
        by moving focus to another widget).
        
        Example: User starts with all spinboxes set to 0. He types 5 in 'Minutes' and
        immediately uses the keyboard to activate the 'Save as Preset' button. Because
        the user never moved focus out of the spinbutton, get_value_as_int() will report
        the previous value of 0, and the saved preset will not have the expected duration
        of 5 minutes.
        
        If a particular spinbutton is empty or contains a non-float value, then this method
        will report its value as zero.
        
        Returns a tuple in this format: (hours, minutes, seconds)
        
        """
        hours_str = self._hours_spin.props.text
        minutes_str = self._minutes_spin.props.text
        seconds_str = self._seconds_spin.props.text
        hours = 0
        minutes = 0
        seconds = 0

        try:
            hours = float(hours_str)
        except TypeError:
            pass
        except ValueError:
            pass

        try:
            minutes = float(minutes_str)
        except TypeError:
            pass
        except ValueError:
            pass

        try:
            seconds = float(seconds_str)
        except TypeError:
            pass
        except ValueError:
            pass

        minutes_fraction = minutes - int(minutes)
        hours_fraction = hours - int(hours)

        # Distribute overflow from seconds to minutes to hours.
        if seconds > 59:
            minutes = math.floor(seconds / 60)
            seconds = seconds % 60
        if minutes > 59:
            hours = math.floor(minutes / 60)
            minutes = minutes % 60
        if hours > DurationChooser.MAX_HOURS:
            hours = DurationChooser.MAX_HOURS

        # Distribute fractions.
        if hours_fraction > 0:
            minutes = int(hours_fraction * 60)
        if minutes_fraction > 0:
            seconds = int(minutes_fraction * 60)

        return (int(hours), int(minutes), int(seconds))
    
    def set_duration(self, hours, minutes, seconds):
        self._hours_spin.set_value(hours)
        self._minutes_spin.set_value(minutes)
        self._seconds_spin.set_value(seconds)

    def normalize_fields(self):
        (hours, minutes, seconds) = self.get_duration()
        self._hours_spin.set_value(hours)
        self._minutes_spin.set_value(minutes)
        self._seconds_spin.set_value(seconds)

    def clear(self):
        self._hours_spin.set_value(0)
        self._minutes_spin.set_value(0)
        self._seconds_spin.set_value(0)
        
    def focus_hours(self):
        self._hours_spin.grab_focus()
        
    def _add_row(self, row_label_str, size_group, max_spin_val):
        row_hbox = gtk.HBox(False, 6)
        
        label = gtk.Label(row_label_str)
        label.set_property('xalign', 0.0)
        label.set_property('use-underline', True)
        size_group.add_widget(label)
        
        spin_adj = gtk.Adjustment(0, 0, max_spin_val, 1, 1, 0)
        spin_button = gtk.SpinButton(spin_adj, 1.0, 0)
        spin_button.props.activates_default = True
        spin_button.props.numeric = False
        spin_button.props.update_policy = gtk.UPDATE_IF_VALID
        
        label.set_mnemonic_widget(spin_button)
        
        row_hbox.pack_start(label, False, False, 0)
        row_hbox.pack_start(spin_button, True, True, 0)
        self.pack_start(row_hbox, False, False, 0)
        
        spin_button.connect('changed', self._on_spin_button_val_changed)
        spin_button.connect('focus-out-event', self._on_spin_button_focus_out)
        spin_button.connect('activate', self._on_spin_button_activate)
        
        label.show()
        spin_button.show()
        row_hbox.show()
        return spin_button
        
    def _on_spin_button_val_changed(self, spin_button):
        self.emit('duration-changed')

    def _on_spin_button_focus_out(self, spin_button, event):
        self.normalize_fields()

    def _on_spin_button_activate(self, entry):
        self.normalize_fields()
