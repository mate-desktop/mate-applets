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
import gtk
import gtk.glade as glade

class ManagePresetsDialog(gobject.GObject):
    __gsignals__ = {'clicked-add':
                        (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, ()),
                    'clicked-edit':
                        (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_PYOBJECT,)),
                    'clicked-remove':
                        (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_PYOBJECT,))}

    def __init__(self, glade_file_name, presets_store, preset_display_func):
        gobject.GObject.__init__(self)
        
        glade_widgets = glade.XML(glade_file_name, 'manage_presets_dialog')
        self._dialog = glade_widgets.get_widget('manage_presets_dialog')
        self._presets_view = glade_widgets.get_widget('presets_view')
        self._delete_button = glade_widgets.get_widget('delete_button')
        self._edit_button = glade_widgets.get_widget('edit_button')
        self._add_button = glade_widgets.get_widget('add_button')

        self._presets_view.set_model(presets_store)
        renderer = gtk.CellRendererText()
        col = gtk.TreeViewColumn('Preset', renderer)
        
        def preset_cell_data_func(col, cell, model, row_iter, user_data=None):
            cell.props.text = preset_display_func(row_iter)
            
        col.set_cell_data_func(renderer, preset_cell_data_func)
        self._presets_view.append_column(col)
        
        self._dialog.connect('response', self._on_dialog_response)
        self._dialog.connect('delete-event', self._dialog.hide_on_delete)
        self._presets_view.get_selection().connect('changed', lambda selection: self._update_button_states())
        self._delete_button.connect('clicked', self._on_delete_button_clicked)
        self._edit_button.connect('clicked', self._on_edit_button_clicked)
        self._add_button.connect('clicked', self._on_add_button_clicked)
        
        self._update_button_states()
        self._dialog.set_default_size(300, 220)
    
    def show(self):
        self._dialog.present()
    
    def _get_selected_path(self):
        selection = self._presets_view.get_selection()
        (model, selection_iter) = selection.get_selected()
        return model.get_path(selection_iter)
        
    def _update_button_states(self):
        selection = self._presets_view.get_selection()
        num_selected = selection.count_selected_rows()
        self._delete_button.props.sensitive = num_selected >= 1
        self._edit_button.props.sensitive = num_selected == 1
    
    def _on_delete_button_clicked(self, button):
        row_path = self._get_selected_path()
        self.emit('clicked-remove', row_path)
    
    def _on_edit_button_clicked(self, button):
        row_path = self._get_selected_path()
        self.emit('clicked-edit', row_path)
    
    def _on_add_button_clicked(self, button):
        self.emit('clicked-add')
        
    def _on_dialog_response(self, dialog, response_id):
        dialog.hide()

