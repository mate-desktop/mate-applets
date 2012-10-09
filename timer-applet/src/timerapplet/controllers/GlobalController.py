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
import gtk
from timerapplet import core
from timerapplet import ui
from timerapplet import utils
from timerapplet import config

class GlobalController(object):
    def __init__(self):
        self._presets_store = core.PresetsStore(config.PRESETS_PATH)
        self._manage_presets_dialog = ui.ManagePresetsDialog(config.GLADE_PATH,
                                                             self._presets_store.get_model(),
                                                             lambda row_iter: utils.get_preset_display_text(self._presets_store,
                                                                                                            row_iter))
        
        self._manage_presets_dialog.connect('clicked-add', self._on_mgr_clicked_add)
        self._manage_presets_dialog.connect('clicked-edit', self._on_mgr_clicked_edit)
        self._manage_presets_dialog.connect('clicked-remove', self._on_mgr_clicked_remove)
        
        gtk.window_set_default_icon_from_file(config.ICON_PATH)

    def get_presets_store(self):
        return self._presets_store
        
    def get_manage_presets_dialog(self):
        return self._manage_presets_dialog

    def _on_mgr_clicked_add(self, sender, data=None):
        add_dialog = ui.AddEditPresetDialog(
            config.GLADE_PATH,
            _('Add Preset'),
            lambda name: utils.is_valid_preset_name(name, self._presets_store))
        
        result = add_dialog.get_preset()
        if result is not None:
            (name, hours, minutes, seconds, command, next_timer, auto_start) = result
            self._presets_store.add_preset(name, hours, minutes, seconds,
                                           command, next_timer, auto_start)
        
    def _on_mgr_clicked_edit(self, sender, row_path, data=None):
        row_iter = self._presets_store.get_model().get_iter(row_path)
        (name, hours, minutes, seconds, command, next_timer, auto_start) = \
                self._presets_store.get_preset(row_iter)

        edit_dialog = ui.AddEditPresetDialog(config.GLADE_PATH,
                                             _('Edit Preset'),
                                             lambda name: utils.is_valid_preset_name(name,
                                                                                     self._presets_store,
                                                                                     (name,)),
                                             name,
                                             hours,
                                             minutes,
                                             seconds,
                                             command,
                                             next_timer,
                                             auto_start
                                            )
            
        result = edit_dialog.get_preset()
        if result is not None:
            (name, hours, minutes, seconds, command, next_timer, auto_start) = result
            self._presets_store.modify_preset(row_iter, name, hours, minutes,
                                              seconds, command, next_timer,
                                              auto_start)

    def _on_mgr_clicked_remove(self, sender, row_path, data=None):
        row_iter = self._presets_store.get_model().get_iter(row_path)
        self._presets_store.remove_preset(row_iter)

    # TODO
    def _on_mgr_next_timer_is_being_edited(self, sender, row_path, data=None):
        """Show a dropdown widget to help completing the next timer."""
        raise NotImplementedError("Not implemented, yet")


