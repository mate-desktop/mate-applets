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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

try:
    from xml.etree import ElementTree as et
except:
    from elementtree import ElementTree as et

import os
from os import path
import gobject
import gtk
import timerapplet.utils as utils

from timerapplet.utils import (serialize_bool,
                               deserialize_bool,
                               seconds_to_hms,
                               hms_to_seconds)
from timerapplet.defs import VERSION

class PersistentStore(gtk.ListStore):
    def __init__(self, load_func, save_func, *args):
        gtk.ListStore.__init__(self, *args)
        load_func(self)
        
        self.connect('row-deleted', lambda model, row_path: save_func(self))
        self.connect('row-changed', lambda model, row_path, row_iter: save_func(self))

class PresetsStore(gobject.GObject):
    (_NAME_COL,
     _HOURS_COL,
     _MINUTES_COL,
     _SECONDS_COL,
     _COM_COL,
     _NEXT_COL,
     _AUTO_START_COL) = xrange(7)
    
    def __init__(self, filename):
        object.__init__(self)
        self._model = PersistentStore(lambda model: PresetsStore._load_presets(model, filename),
                                      lambda model: PresetsStore._save_presets(model, filename),
                                      gobject.TYPE_STRING,
                                      gobject.TYPE_INT,
                                      gobject.TYPE_INT,
                                      gobject.TYPE_INT,
                                      gobject.TYPE_STRING,
                                      gobject.TYPE_STRING,
                                      gobject.TYPE_BOOLEAN,
                                     )
        
    def get_model(self):
        """Return GtkTreeModel.
        
        Should not rely on it being any particular subtype of GtkTreeModel.
        
        """
        return self._model
    
    def get_preset(self, row_iter):
        return self._model.get(row_iter,
                               PresetsStore._NAME_COL,
                               PresetsStore._HOURS_COL,
                               PresetsStore._MINUTES_COL,
                               PresetsStore._SECONDS_COL,
                               PresetsStore._COM_COL,
                               PresetsStore._NEXT_COL,
                               PresetsStore._AUTO_START_COL,
                              )
    
    def add_preset(self, name, hours, minutes, seconds, command, next_timer,
                   auto_start):
        self._model.append((name, hours, minutes, seconds, command, next_timer,
                           auto_start))
        
    def modify_preset(self, row_iter, name, hours, minutes, seconds, command,
                      next_timer, auto_start):
        self._model.set(row_iter,
                        PresetsStore._NAME_COL, name,
                        PresetsStore._HOURS_COL, hours,
                        PresetsStore._MINUTES_COL, minutes,
                        PresetsStore._SECONDS_COL, seconds,
                        PresetsStore._COM_COL, command,
                        PresetsStore._NEXT_COL, next_timer,
                        PresetsStore._AUTO_START_COL, auto_start
                       )
    
    def remove_preset(self, row_iter):
        self._model.remove(row_iter)
    
    def preset_name_exists_case_insensitive(self, preset_name):
        preset_name = preset_name.lower()
        for preset in self._model:
            if preset_name == preset[PresetsStore._NAME_COL].lower():
                return True
        return False
    
    def _load_presets(model, file_path):
        try:
            tree = et.parse(file_path)
        except:
            return
            
        root = tree.getroot()
        
        for node in root:
            name = node.get('name')
            (hours, minutes, seconds) = seconds_to_hms(int(node.get('duration')))
            command = node.get('command')
            next_timer = node.get('next_timer')
            auto_start = node.get('auto_start')
            model.append((name, hours, minutes, seconds, command, next_timer,
                          deserialize_bool(auto_start)))
    _load_presets = staticmethod(_load_presets)

    def _save_presets(model, file_path):
        root = et.Element('timerapplet')
        root.set('version', VERSION)
        
        def add_xml_node(model, path, row_iter):
            (name, hours, minutes, seconds, command, next_timer, auto_start) = \
                    model.get(row_iter, 
                              PresetsStore._NAME_COL,
                              PresetsStore._HOURS_COL,
                              PresetsStore._MINUTES_COL,
                              PresetsStore._SECONDS_COL, 
                              PresetsStore._COM_COL,
                              PresetsStore._NEXT_COL,
                              PresetsStore._AUTO_START_COL
                             )
            node = et.SubElement(root, 'preset')
            node.set('name', name)
            node.set('duration', str(hms_to_seconds(hours, minutes, seconds)))
            node.set('command', command or '')
            node.set('next_timer', next_timer or '')
            node.set('auto_start', serialize_bool(auto_start))
        
        model.foreach(add_xml_node)
        tree = et.ElementTree(root)
        
        file_dir = path.dirname(file_path)
        if not path.exists(file_dir):
            print 'Creating config directory: %s' % file_dir
            os.makedirs(file_dir, 0744)
        tree.write(file_path)
    _save_presets = staticmethod(_save_presets)

