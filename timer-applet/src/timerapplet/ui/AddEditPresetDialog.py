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

import gtk
import gtk.glade as glade
from DurationChooser import DurationChooser

class AddEditPresetDialog(object):
    def __init__(self, glade_file_name, title, name_validator_func,
                 name='', hours=0, minutes=0, seconds=0, command='',
                 next_timer='', auto_start=False):
        self._valid_name_func = name_validator_func
        
        glade_widgets = glade.XML(glade_file_name, 'add_edit_preset_dialog')
        self._dialog = glade_widgets.get_widget('add_edit_preset_dialog')
        self._ok_button = glade_widgets.get_widget('ok_button')
        self._cancel_button = glade_widgets.get_widget('cancel_button')
        self._name_entry = glade_widgets.get_widget('name_entry')
        duration_chooser_container = glade_widgets.get_widget('duration_chooser_container')
        self._duration_chooser = DurationChooser(gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL))
        self._command_entry = glade_widgets.get_widget('command_entry')
        self._next_timer_entry = glade_widgets.get_widget('next_timer_entry')
        self._auto_start_check = glade_widgets.get_widget('auto_start_check')
        
        duration_chooser_container.pack_start(self._duration_chooser)
        
        self._dialog.set_title(title)
        self._dialog.set_default_response(gtk.RESPONSE_OK)
        self._name_entry.set_text(name)
        self._command_entry.set_text(command)
        self._duration_chooser.set_duration(hours, minutes, seconds)
        self._next_timer_entry.set_text(next_timer)
        self._auto_start_check.set_active(auto_start)
        
        self._name_entry.connect('changed', lambda entry: self._check_for_valid_save_preset_input())
        self._duration_chooser.connect('duration-changed',
                                       lambda chooser: self._check_for_valid_save_preset_input())
        self._duration_chooser.show()
    
    def _non_zero_duration(self):
        (hours, minutes, seconds) = self._duration_chooser.get_duration()
        return (hours > 0 or minutes > 0 or seconds > 0)
        
    def _check_for_valid_save_preset_input(self):
        self._ok_button.props.sensitive = (self._non_zero_duration() and 
                                           self._valid_name_func(self._name_entry.get_text()))

    ## Callback for saving ##

    def get_preset(self):
        self._check_for_valid_save_preset_input()
        result = self._dialog.run()
        self._dialog.hide()
        if result == gtk.RESPONSE_OK:
            (hours, minutes, seconds) = self._duration_chooser.get_duration()
            cmd = self._command_entry.get_text()
            next_timer = self._next_timer_entry.get_text()
            auto_start = self._auto_start_check.get_active()
            return (self._name_entry.get_text(), hours, minutes, seconds, cmd,
                    next_timer, auto_start)
        else:
            return None
