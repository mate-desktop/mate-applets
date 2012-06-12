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

class PreferencesDialog(gobject.GObject):
    __gsignals__ = \
    {
        'show-remaining-time-changed': 
        (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            (gobject.TYPE_BOOLEAN,)
        ),
        'play-sound-changed':
        (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            (gobject.TYPE_BOOLEAN,)
        ),
        'use-custom-sound-changed':
        (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            (gobject.TYPE_BOOLEAN,)
        ),
        'show-popup-notification-changed':
        (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            (gobject.TYPE_BOOLEAN,)
        ),
        'show-pulsing-icon-changed':
        (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            (gobject.TYPE_BOOLEAN,)
        ),
        'custom-sound-path-changed':
        (
            gobject.SIGNAL_RUN_LAST,
            gobject.TYPE_NONE,
            (gobject.TYPE_STRING,)
        )
    }

    __gproperties__ = \
    {
        'show-remaining-time':
        (bool,
         'Show remaining time',
         'Whether to show remaining time when the timer is running',
         False,
         gobject.PARAM_WRITABLE
        ),
       'play-sound':
        (bool,
         'Play notification sound',
         'Whether to play notification sound when the timer is finished',
         False,
         gobject.PARAM_WRITABLE
        ),
       'use-custom-sound':
        (bool,
         'Use custom sound',
         'Whether to use a custom notification sound',
         False,
         gobject.PARAM_WRITABLE
        ),
       'show-popup-notification':
        (bool,
         'Show Popup notification',
         'Whether to show a popup notifcation when timer finished', 
         False,
         gobject.PARAM_WRITABLE
        ), 
       'show-pulsing-icon':
        (bool,
         'Show pulsing icon',
         'Whether to show pulsing icon when timer finished', 
         False,
         gobject.PARAM_WRITABLE
        ), 
       'custom-sound-path':
        (str,
         'Custom sound path',
         'Path to a custom notification sound',
         '',
         gobject.PARAM_WRITABLE
        )
    }
                        
    def __init__(self, glade_file_name):
        gobject.GObject.__init__(self)
        glade_widgets = glade.XML(glade_file_name, 'preferences_dialog')
        self._preferences_dialog = glade_widgets.get_widget('preferences_dialog')
        self._show_time_check = glade_widgets.get_widget('show_time_check')
        self._play_sound_check = glade_widgets.get_widget('play_sound_check')
        self._use_default_sound_radio = glade_widgets.get_widget('use_default_sound_radio')
        self._use_custom_sound_radio = glade_widgets.get_widget('use_custom_sound_radio')
        self._sound_chooser_button = glade_widgets.get_widget('sound_chooser_button')
        self._play_sound_box = glade_widgets.get_widget('play_sound_box')
        #: Popup notification checkbutton
        self._popup_notification_check = glade_widgets.get_widget('popup_notification_check')
        #: Pulsing icon checkbutton
        self._pulsing_icon_check = glade_widgets.get_widget('pulsing_trayicon_check')

        #######################################################################
        # Signals
        #######################################################################
        self._show_time_check.connect('toggled', self._on_show_time_check_toggled)
        self._play_sound_check.connect('toggled', self._on_play_sound_check_toggled)
        self._use_custom_sound_radio.connect('toggled', self._on_use_custom_sound_radio_toggled)
        #: Popup notification checkbutton 'toggled' signal
        self._popup_notification_check.connect('toggled', self._on_popup_notification_toggled)
        #: Pulsing icon checkbutton 'toggled' signal
        self._pulsing_icon_check.connect('toggled', self._on_pulsing_icon_toggled)
        
        self._sound_chooser_button.connect('selection-changed', self._on_sound_chooser_button_selection_changed)
        self._preferences_dialog.connect('delete-event', gtk.Widget.hide_on_delete)
        self._preferences_dialog.connect('response', lambda dialog, response_id: self._preferences_dialog.hide())
    
    def show(self):
        self._preferences_dialog.present()
    
    def _on_show_time_check_toggled(self, widget):
        self.emit('show-remaining-time-changed', widget.props.active)
        
    def _on_play_sound_check_toggled(self, widget):
        self.emit('play-sound-changed', widget.props.active)
        
    def _on_use_custom_sound_radio_toggled(self, widget):
        self.emit('use-custom-sound-changed', widget.props.active)

    def _on_popup_notification_toggled(self, widget):
        """Emit a signal when `self._popup_notification_check` gets toggled in
        the Preferences dialog window."""
        self.emit('show-popup-notification-changed', widget.props.active)
        
    def _on_pulsing_icon_toggled(self, widget):
        """Emit a signal when `self._popup_notification_check` gets toggled in
        the Preferences dialog window."""
        self.emit('show-pulsing-icon-changed', widget.props.active)
        
    def _on_sound_chooser_button_selection_changed(self, chooser_button):
        filename = chooser_button.get_filename()
        
        # Work around an issue where calling set_filename() will cause
        # 3 selection-changed signals to be emitted: first two will have a None filename
        # and the third will finally have the desired filename.
        if filename is not None:
            print 'Custom sound changed to path: %s' % filename
            self.emit('custom-sound-path-changed', filename)

    def do_set_property(self, pspec, value):
        if pspec.name == 'show-remaining-time':
            self._show_time_check.props.active = value
        elif pspec.name == 'play-sound':
            self._play_sound_check.props.active = value
            self._play_sound_box.props.sensitive = value
        elif pspec.name == 'use-custom-sound':
            if value == True:
                self._use_custom_sound_radio.props.active = True
                self._sound_chooser_button.props.sensitive = True
            else:
                # Note: Setting _use_custom_sound_radio.props.active to False
                # does not automatically set _use_default_sound_radio.props.active to True
                self._use_default_sound_radio.props.active = True
                self._sound_chooser_button.props.sensitive = False
        elif pspec.name == 'show-popup-notification':
            self._popup_notification_check.props.active = value
        elif pspec.name == 'show-pulsing-icon':
            self._pulsing_icon_check.props.active = value
        elif pspec.name == 'custom-sound-path':
            # Prevent infinite loop of events.
            if self._sound_chooser_button.get_filename() != value:
                self._sound_chooser_button.set_filename(value)
