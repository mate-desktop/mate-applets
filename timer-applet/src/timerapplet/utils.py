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

from gettext import gettext as _

def is_valid_preset_name(name_str, preset_store, allowed_names=()):
    if len(name_str) == 0:
        return False
        
    name_str = name_str.lower()
    for allowed in allowed_names:
        if name_str == allowed.lower():
            return True
    
    return not preset_store.preset_name_exists_case_insensitive(name_str)
                
def seconds_to_hms(total_seconds):
    (hours, remaining_seconds) = divmod(total_seconds, 3600)
    (minutes, seconds) = divmod(remaining_seconds, 60)
    return (hours, minutes, seconds)
    
def hms_to_seconds(hours, minutes, seconds):
    return hours * 3600 + minutes * 60 + seconds
    
def construct_time_str(remaining_seconds, show_all=True):
    """Return a user-friendly representation of remaining time based on the given number of seconds.
    
    show_all specifies whether the returned string should show all time components.
    If show_all is True (default), the returned string is in HH:MM:SS format.
    If show_all is False, the returned string is in either HH:MM or MM:SS format,
    depending on how much time is remaining. This avoids showing the user more
    information than necessary.
    
    """
    hours, minutes, seconds = seconds_to_hms(remaining_seconds)
    if show_all:
        # HH:MM:SS
        return _('%02d:%02d:%02d') % (hours, minutes, seconds)
    else:
        if hours > 0 or minutes > 14:
            # HH:MM
            return _('%02d:%02d') % (hours, minutes)
        else:
            # MM:SS
            return _('%02d:%02d') % (minutes, seconds)

def get_display_text_from_datetime(date_time):
    return date_time.strftime('%X')

def get_preset_display_text(presets_store, row_iter):
    (name, hours, minutes, seconds, command, next_timer, auto_start) = \
            presets_store.get_preset(row_iter)
    
    # <preset name> (HH:MM:SS)
    return _('%s (%02d:%02d:%02d)') % (name, hours, minutes, seconds)

def serialize_bool(boolean):
    if boolean:
        return "1"
    return "0"

def deserialize_bool(string):
    if string == "1":
        return True
    return False
