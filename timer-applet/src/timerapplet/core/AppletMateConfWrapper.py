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

from os import path
import mateconf

class AppletMateConfWrapper(object):
    def __init__(self, applet, schema_path, standalone_key):
        object.__init__(self)
        self._connection_ids = []
        
        self._client = mateconf.client_get_default()
        
        # Get preferences key path for the given applet instance.
        self._base_path = applet.get_preferences_key()
        if self._base_path is not None:
            # Apply the schema to the applet instance preferences key.
            applet.add_preferences(schema_path)
        else:
            # NOTE: Don't need to apply schema here because the Timer Applet schema file
            # already specifies that the schema be automatically applied to the standalone key.
            
            self._base_path = standalone_key
            
            # Applet would usually do this for us, but since we're running in standalone mode,
            # we have to do this ourselves in order to receive MateConf change notifications.
            self._client.add_dir(self._base_path, mateconf.CLIENT_PRELOAD_RECURSIVE) 
            
        print 'Base prefs path = %s' % self._base_path
        
    def get_base_path(self):
        return self._base_path
        
    def add_notification(self, relative_key, callback, data=None):
        """Register for notifications of changes to the given preference key.
        
        relative_key should be relative to the applet's base preferences key path.
        callback should look like: callback(MateConfValue, data=None)
        """
        connection_id = self._client.notify_add(self._get_full_path(relative_key),
                                                self._notification_callback, (callback, data))
        self._connection_ids.append(connection_id)
    
    def get_string(self, relative_key):
        return self._client.get_string(self._get_full_path(relative_key))
        
    def get_bool(self, relative_key):
        return self._client.get_bool(self._get_full_path(relative_key))
    
    def set_string(self, relative_key, val):
        self._client.set_string(self._get_full_path(relative_key), val)
        
    def set_bool(self, relative_key, val):
        self._client.set_bool(self._get_full_path(relative_key), val)
        
    def delete(self):
        for connection_id in self._connection_ids:
            self._client.notify_remove(connection_id)
        self._connection_ids = []
            
    def _notification_callback(self, client, cnxn_id, entry, data=None):
        (callback, real_data) = data
        
        # mateconf_value is of type MateConfValue (mateconf.Value)
        mateconf_value = entry.get_value()
        
        # Ignore when mateconf_value is None because that
        # means that the settings are being removed
        # because the applet has been removed from
        # the panel.
        if mateconf_value != None:
            callback(mateconf_value, real_data)
            
    def _get_full_path(self, relative_key):
        return path.join(self._base_path, relative_key)
        
