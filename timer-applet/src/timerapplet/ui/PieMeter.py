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

import math
import gobject
import gtk

class PieMeter(gtk.Image):
    _DEFAULT_SIZE = 24

    def __init__(self):
        gtk.Image.__init__(self)
        self._progress = 0.0
        self._fill_color = (0.0, 1.0, 0.0)
        
    def set_progress(self, progress):
        assert progress >= 0.0
        assert progress <= 1.0
        self._progress = progress
        if self.window is not None:
            self.window.invalidate_rect(self.allocation, True)
        
    def set_fill_color(self, red, green, blue):
        assert 0.0 <= red <= 1.0
        assert 0.0 <= green <= 1.0
        assert 0.0 <= blue <= 1.0
        self._fill_color = (red, green, blue)
        
        if self.window is not None:
            self.window.invalidate_rect(self.allocation, True)
            
    def do_size_request(self, requisition):
        requisition.width = PieMeter._DEFAULT_SIZE
        requisition.height = PieMeter._DEFAULT_SIZE
        
    def do_expose_event(self, event):
        context = event.window.cairo_create()
        
        rect = self.allocation
        x = rect.x + (rect.width / 2)
        y = rect.y + (rect.height / 2)
        radius = (min(rect.width, rect.height) / 2)
        
        # Draw background circle
        context.arc(x, y, radius, 0, 2 * math.pi)
        context.set_source_rgba(0.8, 0.8, 0.8)
        context.fill()
        
        # Draw pie
        context.arc(x, y, radius, (-0.5 * math.pi) + self._progress * 2 * math.pi, 1.5 * math.pi)
        context.line_to(x, y)
        context.close_path()
        (red, green, blue) = self._fill_color
        context.set_source_rgb(red, green, blue)
        context.fill()
        
        # Draw circle outline
        context.arc(x, y, radius, 0, 2 * math.pi)
        context.set_source_rgba(1, 1, 1)
        context.set_line_width(1.0)
        context.stroke()

gobject.type_register(PieMeter)

