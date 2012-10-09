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

def get_scroll_value_to_reveal_widget(widget_rect, viewport_rect, scroll_val):
    if widget_rect.y < scroll_val:
        scroll_val = widget_rect.y
    elif widget_rect.y + widget_rect.height + 1 > scroll_val + viewport_rect.height:
        scroll_val = widget_rect.y + widget_rect.height + 1 - viewport_rect.height    
    return scroll_val

class ScrollableButtonList(gtk.ScrolledWindow):
    def __init__(self):
        gtk.ScrolledWindow.__init__(self)
        
        self._vadjust = gtk.Adjustment()
        self._vbox = gtk.VBox()
        self._viewport = gtk.Viewport()
        
        self.set_vadjustment(self._vadjust)
        self.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
        self._viewport.modify_bg(gtk.STATE_NORMAL, self.style.base[gtk.STATE_NORMAL])
        
        self._viewport.add(self._vbox)
        self.add(self._viewport)
        
        self._vbox.show()
        self._viewport.show()
        
    def add_button(self, button):
        button.connect('focus-in-event', self._on_focus)
        button.connect('clicked', self._on_clicked)
        self._vbox.pack_start(button, False, False)
        
    def get_buttons(self):
        return self._vbox.get_children()
        
    def _on_focus(self, widget, event):
        self._scroll_to_widget(widget)
        
    def _on_clicked(self, widget):
        self._scroll_to_widget(widget)
        
    def _scroll_to_widget(self, widget):
        cur_val = self._vadjust.get_value()
        new_val = get_scroll_value_to_reveal_widget(widget.allocation, self._viewport.allocation, cur_val)
        
        if new_val != cur_val:
            self._vadjust.set_value(new_val)
            self._vadjust.value_changed()
    
