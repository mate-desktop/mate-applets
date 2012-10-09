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

import gtk
from PulseButton import PulseButton
from PieMeter import PieMeter

class StatusButton(PulseButton):
    def __init__(self):
        PulseButton.__init__(self)
        
        self._tooltip = gtk.Tooltip()

        self._icon_widget = gtk.Image()        
        self._pie_meter = PieMeter()
        self._label_widget = gtk.Label()
        self._visual_box = gtk.HBox()
        
        self._visual_box.pack_start(self._icon_widget)
        self._visual_box.pack_start(self._pie_meter)
        
        self._layout_box = None
        self._use_vertical = None
        self.set_use_vertical_layout(False)
        
        # _pie_meter will default to visible while
        # _icon_widget will default to hidden.
        self._pie_meter.show()
        self._visual_box.show()
        self._label_widget.show()
    
    def set_tooltip(self, tip_text):
        self._tooltip.set_text(tip_text)
    
    def set_label(self, text):
        self._label_widget.set_text(text)
        
    def set_icon(self, image_path):
        self._icon_widget.set_from_pixbuf(gtk.gdk.pixbuf_new_from_file_at_size(image_path, -1, 20))
        #elf._icon_widget.set_from_file(image_path)
        
    def set_use_icon(self, use_icon):
        if use_icon:
            self._pie_meter.hide()
            self._icon_widget.show()
        else:
            self._pie_meter.show()
            self._icon_widget.hide()
        
    def set_sensitized(self, sensitized):
        self._label_widget.props.sensitive = sensitized
        
    def set_show_remaining_time(self, show_remaining_time):
        if show_remaining_time:
            self._label_widget.show()
        else:
            self._label_widget.hide()
            
    def set_progress(self, progress):
        self._pie_meter.set_progress(progress)

    def set_use_vertical_layout(self, use_vertical):
        if self._use_vertical == use_vertical:
            return
        
        self._use_vertical = use_vertical
        if self._layout_box is not None:
            self._layout_box.remove(self._visual_box)
            self._layout_box.remove(self._label_widget)
            self.remove(self._layout_box)
            self._layout_box.destroy()
            self._layout_box = None
        
        new_layout_box = None
        if self._use_vertical:
            new_layout_box = gtk.VBox(False, 2)
        else:
            new_layout_box = gtk.HBox(False, 2)
        
        new_layout_box.pack_start(self._visual_box, True, True, 0)
        new_layout_box.pack_start(self._label_widget, False, False, 0)
        
        self._layout_box = new_layout_box
        self.add(self._layout_box)
        self._layout_box.show()
        
    def set_pie_fill_color(self, red, green, blue):
        self._pie_meter.set_fill_color(red, green, blue)
