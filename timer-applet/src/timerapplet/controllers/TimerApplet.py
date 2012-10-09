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
from gettext import ngettext
from datetime import datetime, timedelta
import mateapplet
import gst
import gtk
import gtk.glade as glade
import gtk.gdk as gdk
import subprocess
import shlex
import threading
from timerapplet import config
from timerapplet import core
from timerapplet import ui
from timerapplet import utils

def on_widget_button_press_event(sender, event, data=None):
    if event.button != 1:
        sender.emit_stop_by_name('button-press-event')  
    return False

def force_no_focus_padding(widget):
    gtk.rc_parse_string('\n'
                        '   style "timer-applet-button-style"\n'
                        '   {\n'
                        '      GtkWidget::focus-line-width=0\n'
                        '      GtkWidget::focus-padding=0\n'
                        '   }\n'
                        '\n'
                        '   widget "*.timer-applet-button" style "timer-applet-button-style"\n'
                        '\n')
    widget.set_name('timer-applet-button')

class TimerApplet(object):
    # MateConf key identifiers
    ## You can find Timer Applet's schemas file in data/timer-applet.schemas.in
    _SHOW_REMAINING_TIME_KEY = 'show_remaining_time'
    _PLAY_SOUND_KEY = 'play_notification_sound'
    _USE_CUSTOM_SOUND_KEY = 'use_custom_notification_sound'
    _SHOW_POPUP_NOTIFICATION_KEY = 'show_popup_notification'
    _SHOW_PULSING_ICON_KEY = 'show_pulsing_icon'
    _CUSTOM_SOUND_PATH_KEY = 'custom_notification_sound_path'
    
    _PRESETS_PLACEHOLDER_NAME = 'Placeholder'
    _PRESETS_PLACEHOLDER_PATH = '/popups/popup/Presets/' + _PRESETS_PLACEHOLDER_NAME
    _PRESETS_PATH = '/popups/popup/Presets'
   
    def __init__(self, presets_store, manage_presets_dialog, applet, timer, mateconf_wrapper):
        self._presets_store = presets_store
        self._manage_presets_dialog = manage_presets_dialog
        self._applet = applet
        self._timer = timer

        self._gst_playbin = gst.element_factory_make('playbin', 'player')
        def bus_event(bus, message):
            t = message.type
            if t == gst.MESSAGE_EOS:
                self._gst_playbin.set_state(gst.STATE_NULL)
            elif t == gst.MESSAGE_ERROR:
                self._gst_playbin.set_state(gst.STATE_NULL)
                err, debug = message.parse_error()
                print 'Error playing sound: %s' % err, debug
            return True
        self._gst_playbin.get_bus().add_watch(bus_event)
        
        self._status_button = ui.StatusButton()
        self._notifier = ui.Notifier('TimerApplet', gtk.STOCK_DIALOG_INFO, self._status_button)
        self._start_next_timer_dialog = ui.StartNextTimerDialog(
            config.GLADE_PATH,
            "Start next timer",
            "Would you like to start the next timer?")
        self._start_timer_dialog = ui.StartTimerDialog(config.GLADE_PATH,
                                                       lambda name: utils.is_valid_preset_name(name,
                                                                                               self._presets_store),
                                                       self._presets_store.get_model(),
                                                       lambda row_iter: utils.get_preset_display_text(self._presets_store,
                                                                                                      row_iter))
        self._continue_dialog = ui.ContinueTimerDialog(config.GLADE_PATH,
                                                       _('Continue timer countdown?'),
                                                       _('The timer is currently paused. Would you like to continue countdown?'))
        self._preferences_dialog = ui.PreferencesDialog(config.GLADE_PATH)
        self._mateconf = mateconf_wrapper
        
        self._about_dialog = glade.XML(config.GLADE_PATH, 'about_dialog').get_widget('about_dialog')
        self._about_dialog.set_version(config.VERSION)
        
        self._applet.set_applet_flags(mateapplet.EXPAND_MINOR)
        self._applet.setup_menu_from_file(
            None,
            config.POPUP_MENU_FILE_PATH,
            None,
            [('PauseTimer', lambda component, verb: self._timer.stop()),
             ('ContinueTimer', lambda component, verb: self._timer.start()),
             ('StopTimer', lambda component, verb: self._timer.reset()),
             ('RestartTimer', lambda component, verb: self._restart_timer()),
             ('StartNextTimer', lambda component, verb: self._start_next_timer()),
             ('ManagePresets', lambda component, verb: self._manage_presets_dialog.show()),
             ('Preferences', lambda component,
              verb: self._preferences_dialog.show()),
             ('About', lambda component, verb: self._about_dialog.show())]
        )
        self._applet.add(self._status_button)
        
        # Remove padding around button contents.
        force_no_focus_padding(self._status_button)
        
        # TODO:
        # Fix bug in which button would not propogate middle-clicks
        # and right-clicks to the applet.
        self._status_button.connect('button-press-event', on_widget_button_press_event)
        
        self._status_button.set_relief(gtk.RELIEF_NONE)
        self._status_button.set_icon(config.ICON_PATH);
        
        self._connect_signals()
        self._update_status_button()
        self._update_popup_menu()
        self._update_preferences_dialog()
        self._status_button.show()
        self._applet.show()
    
    def _connect_signals(self):
        self._applet.connect('change-orient', lambda applet, orientation: self._update_status_button())
        self._applet.connect('change-size', lambda applet, size: self._update_status_button())
        self._applet.connect('change-background', self._on_applet_change_background)
        self._applet.connect('destroy', self._on_applet_destroy)
        
        self._presets_store.get_model().connect('row-deleted', 
                                                lambda model,
                                                row_path: self._update_popup_menu())
        self._presets_store.get_model().connect('row-changed',
                                                lambda model,
                                                row_path,
                                                row_iter: self._update_popup_menu())
        
        self._timer.connect('time-changed', self._on_timer_time_changed)
        self._timer.connect('state-changed', self._on_timer_state_changed)
        self._status_button.connect('clicked', self._on_status_button_clicked)
        self._start_timer_dialog.connect('clicked-start',
                                         self._on_start_dialog_clicked_start)
        self._start_timer_dialog.connect('clicked-manage-presets',
                                         self._on_start_dialog_clicked_manage_presets)
        self._start_timer_dialog.connect('clicked-save',
                                         self._on_start_dialog_clicked_save)
        self._start_timer_dialog.connect('clicked-preset',
                                         self._on_start_dialog_clicked_preset)
        self._start_timer_dialog.connect('double-clicked-preset',
                                         self._on_start_dialog_double_clicked_preset)
        
        self._preferences_dialog.connect('show-remaining-time-changed', self._on_prefs_show_time_changed)
        self._preferences_dialog.connect('play-sound-changed', self._on_prefs_play_sound_changed)
        self._preferences_dialog.connect('use-custom-sound-changed', self._on_prefs_use_custom_sound_changed)
        self._preferences_dialog.connect('show-popup-notification-changed', self._on_prefs_show_popup_notification_changed)
        self._preferences_dialog.connect('show-pulsing-icon-changed', self._on_prefs_show_pulsing_icon_changed)
        self._preferences_dialog.connect('custom-sound-path-changed', self._on_prefs_custom_sound_path_changed)
        
        self._about_dialog.connect('delete-event', gtk.Widget.hide_on_delete)
        self._about_dialog.connect('response', lambda dialog, response_id: self._about_dialog.hide())

        self._mateconf.add_notification(TimerApplet._SHOW_REMAINING_TIME_KEY, self._on_mateconf_changed)
        self._mateconf.add_notification(TimerApplet._PLAY_SOUND_KEY, self._on_mateconf_changed)
        self._mateconf.add_notification(TimerApplet._USE_CUSTOM_SOUND_KEY, self._on_mateconf_changed)
        self._mateconf.add_notification(TimerApplet._SHOW_PULSING_ICON_KEY, self._on_mateconf_changed)
        self._mateconf.add_notification(TimerApplet._SHOW_POPUP_NOTIFICATION_KEY, self._on_mateconf_changed)
        self._mateconf.add_notification(TimerApplet._CUSTOM_SOUND_PATH_KEY, self._on_mateconf_changed)
    
    ## Private methods for updating UI ##
    
    def _update_status_button(self):
        current_state = self._timer.get_state()
        if current_state == core.Timer.STATE_IDLE:
            print 'Idle'
            # This label text should not be visible because the label
            # is hidden when the timer is idle.
            self._status_button.set_label('--:--:--')
            self._status_button.set_tooltip(_('Click to start a new timer countdown.'))
        elif current_state == core.Timer.STATE_RUNNING:
            print 'Running'
        elif current_state == core.Timer.STATE_PAUSED:
            print 'Paused'
            self._status_button.set_tooltip(_('Paused. Click to continue timer countdown.'))
        elif current_state == core.Timer.STATE_FINISHED:
            print 'Finished'
            self._status_button.set_label(_('Finished'))
            name_str = self._timer.get_name()
            time_str = utils.get_display_text_from_datetime(self._timer.get_end_time())
            if len(name_str) > 0:
                # "<timer name>" finished at <time>
                self._status_button.set_tooltip(_('"%s" finished at %s.\nClick to stop timer.') % (name_str, time_str))
            else:
                # Timer finished at <time>
                self._status_button.set_tooltip(_('Timer finished at %s.\nClick to stop timer.') % time_str)
        
        self._status_button.set_sensitized(current_state == core.Timer.STATE_RUNNING or
                                           current_state == core.Timer.STATE_FINISHED)
        self._status_button.set_use_icon(current_state == core.Timer.STATE_IDLE)
        self._status_button.set_show_remaining_time(current_state != core.Timer.STATE_IDLE and
                self._mateconf.get_bool(TimerApplet._SHOW_REMAINING_TIME_KEY))
        
        if current_state == core.Timer.STATE_PAUSED:
            self._status_button.set_pie_fill_color(0.4, 0.4, 0.4)
        else:
            # Use theme color
            color = self._applet.style.base[gtk.STATE_SELECTED]
            red = color.red / 65535.0
            green = color.green / 65535.0
            blue = color.blue / 65535.0
            self._status_button.set_pie_fill_color(red, green, blue)
        
        orientation = self._applet.get_orient()
        size = self._applet.get_size()
        use_vertical = (orientation == mateapplet.ORIENT_LEFT or
                        orientation == mateapplet.ORIENT_RIGHT or
                        size >= mateapplet.SIZE_MEDIUM)
        self._status_button.set_use_vertical_layout(use_vertical)
    
    def _update_popup_menu(self):
        popup = self._applet.get_popup_component()
        
        timer_state = self._timer.get_state()
        has_next_timer = self._timer.get_next_timer()
        show_pause = (timer_state == core.Timer.STATE_RUNNING)
        show_continue = (timer_state == core.Timer.STATE_PAUSED)
        show_stop = (timer_state == core.Timer.STATE_RUNNING or
                     timer_state == core.Timer.STATE_PAUSED or
                     timer_state == core.Timer.STATE_FINISHED)
        show_restart = (timer_state == core.Timer.STATE_RUNNING or
                        timer_state == core.Timer.STATE_PAUSED or
                        timer_state == core.Timer.STATE_FINISHED)
        show_next_timer = ((timer_state == core.Timer.STATE_RUNNING or
                           timer_state == core.Timer.STATE_PAUSED or
                           timer_state == core.Timer.STATE_FINISHED) and
                           # Only show this popup menu item if it has a
                           # next_timer defined. Clever, huh? ;)
                           has_next_timer)
        
        show_presets_menu = (len(self._presets_store.get_model()) > 0)
        show_separator = (
            show_presets_menu or
            show_pause or
            show_next_timer or
            show_continue or
            show_stop or
            show_restart)
        
        to_hidden_str = lambda show: ('0', '1')[not show]
        popup.set_prop('/commands/PauseTimer', 'hidden', to_hidden_str(show_pause))
        popup.set_prop('/commands/ContinueTimer', 'hidden', to_hidden_str(show_continue))
        popup.set_prop('/commands/StopTimer', 'hidden', to_hidden_str(show_stop))
        popup.set_prop('/commands/RestartTimer', 'hidden', to_hidden_str(show_restart))
        popup.set_prop('/commands/StartNextTimer', 'hidden', to_hidden_str(show_next_timer))
        popup.set_prop(TimerApplet._PRESETS_PATH, 'hidden', to_hidden_str(show_presets_menu))
        popup.set_prop('/popups/popup/Separator1', 'hidden', to_hidden_str(show_separator))
        
        # Rebuild the Presets submenu
        if popup.path_exists(TimerApplet._PRESETS_PLACEHOLDER_PATH):
            popup.rm(TimerApplet._PRESETS_PLACEHOLDER_PATH)
        popup.set_translate(TimerApplet._PRESETS_PATH,
                            '<placeholder name="%s"/>' % TimerApplet._PRESETS_PLACEHOLDER_NAME)
        
        preset_number = 1
        row_iter = self._presets_store.get_model().get_iter_first()
        while row_iter is not None:
            verb = ('Preset_%d' % preset_number)
            preset_number += 1
            display_text = utils.get_preset_display_text(self._presets_store, row_iter)
            node_xml = '<menuitem verb="%s" name="%s" label="%s"/>' % (verb, verb, display_text)
            popup.set_translate(TimerApplet._PRESETS_PLACEHOLDER_PATH, node_xml)
            popup.add_verb(verb,
                           self._on_presets_submenu_item_activated,
                           self._presets_store.get_model().get_path(row_iter))
            row_iter = self._presets_store.get_model().iter_next(row_iter)
    
    def _update_preferences_dialog(self):
        self._preferences_dialog.props.show_remaining_time = \
            self._mateconf.get_bool(TimerApplet._SHOW_REMAINING_TIME_KEY)
        self._preferences_dialog.props.play_sound = \
            self._mateconf.get_bool(TimerApplet._PLAY_SOUND_KEY)
        self._preferences_dialog.props.use_custom_sound = \
            self._mateconf.get_bool(TimerApplet._USE_CUSTOM_SOUND_KEY)
        self._preferences_dialog.props.show_popup_notification = \
            self._mateconf.get_bool(TimerApplet._SHOW_POPUP_NOTIFICATION_KEY)
        self._preferences_dialog.props.show_pulsing_icon = \
            self._mateconf.get_bool(TimerApplet._SHOW_PULSING_ICON_KEY)
        self._preferences_dialog.props.custom_sound_path = \
            self._mateconf.get_string(TimerApplet._CUSTOM_SOUND_PATH_KEY)
    
    ## Applet callbacks ##
    
    def _on_applet_change_background(self, applet, background_type, color, pixmap):
        applet.set_style(None)
        rc_style = gtk.RcStyle()
        applet.modify_style(rc_style)
        
        if background_type == mateapplet.NO_BACKGROUND:
            pass
        elif background_type == mateapplet.COLOR_BACKGROUND:
            applet.modify_bg(gtk.STATE_NORMAL, color)
        elif background_type == mateapplet.PIXMAP_BACKGROUND:
            style = applet.style.copy()
            style.bg_pixmap[gtk.STATE_NORMAL] = pixmap
            applet.set_style(style)
    
    def _on_applet_destroy(self, sender, data=None):
        self._call_notify(show=False)
        if self._timer.get_state() != core.Timer.STATE_IDLE:
            self._timer.reset() # will stop timeout
        self._mateconf.delete()
        
    ## Popup menu callbacks ##
        
    def _on_presets_submenu_item_activated(self, component, verb, row_path):
        # Try hiding the Start Timer dialog, just in case it's open.
        self._start_timer_dialog.hide()
        row_iter = self._presets_store.get_model().get_iter(row_path)
        (name, hours, minutes, seconds, command, next_timer, auto_start) = self._presets_store.get_preset(row_iter)
        self._start_timer_with_settings(name, hours, minutes, seconds, command,
                                       next_timer, auto_start)
    
    ## MateConf callbacks ##
    
    def _on_mateconf_changed(self, mateconf_value, data=None):
        self._update_status_button()
        self._update_preferences_dialog()
    
    ## PreferencesDialog callbacks ##
    
    def _on_prefs_show_time_changed(self, sender, show_time):
        self._mateconf.set_bool(TimerApplet._SHOW_REMAINING_TIME_KEY,
                             show_time)
        
    def _on_prefs_play_sound_changed(self, sender, play_sound):
        self._mateconf.set_bool(TimerApplet._PLAY_SOUND_KEY,
                             play_sound)
        
    def _on_prefs_use_custom_sound_changed(self, sender, use_custom_sound):
        self._mateconf.set_bool(TimerApplet._USE_CUSTOM_SOUND_KEY,
                             use_custom_sound)
    
    def _on_prefs_show_pulsing_icon_changed(self, sender, show_pulsing_icon):
        self._mateconf.set_bool(TimerApplet._SHOW_PULSING_ICON_KEY, 
                             show_pulsing_icon)

    def _on_prefs_show_popup_notification_changed(self, sender,
                                                  show_popup_notification):
        self._mateconf.set_bool(TimerApplet._SHOW_POPUP_NOTIFICATION_KEY,
                             show_popup_notification)
        
    def _on_prefs_custom_sound_path_changed(self, sender, custom_sound_path):
        self._mateconf.set_string(TimerApplet._CUSTOM_SOUND_PATH_KEY, 
                               custom_sound_path)
    
    ## Timer callbacks ##
    
    def _on_timer_time_changed(self, timer):
        hours, minutes, seconds = utils.seconds_to_hms(timer.get_remaining_time())
        print 'Remaining time: %d, %d, %d' % (hours, minutes, seconds)
        name = self._timer.get_name()
        self._status_button.set_label(utils.construct_time_str(self._timer.get_remaining_time(),
                                                         show_all=False))

        fraction_remaining = float(self._timer.get_remaining_time()) / self._timer.get_duration()
        progress = min(1.0, max(0.0, 1.0 - fraction_remaining))
        self._status_button.set_progress(progress)

        if len(name) > 0:
            # HH:MM:SS (<timer name>)
            self._status_button.set_tooltip(_('%02d:%02d:%02d (%s)') % (hours, minutes, seconds, name))
        else:
            # HH:MM:SS
            self._status_button.set_tooltip(_('%02d:%02d:%02d') % (hours, minutes, seconds))
    
    def _on_timer_state_changed(self, timer, data=None):
        # TODO:
        # Refactor me!
        print 'State changed'
        new_state = timer.get_state()
        print '  new state: %d' % new_state
        
        # These actions should be done once upon a state change.
        # That's why they're done here and not in self._update_status_button();
        # self._update_status_button() could be called multiple times
        # while in the same state.
        if new_state == core.Timer.STATE_FINISHED:
            name = self._timer.get_name()
            command = self._timer.get_command()
            end_time = self._timer.get_end_time()
            time_text = utils.get_display_text_from_datetime(end_time)
            summary = None
            message = None
            if len(name) > 0:
                # "<timer name>" Finished
                summary = (_('"%s" Finished') % name)
                
                # "<timer name>" finished at <time>
                message = (_('"%s" finished at %s') % (name, time_text))
            else:
                summary = _('Timer Finished')
            
                # Timer finished at <time>
                message = (_('Timer finished at %s') % time_text)
            
            
            def reminder_message_func():
                elapsed_time = datetime.now() - end_time
                message = None
                if elapsed_time < timedelta(seconds=60):
                    message = ngettext('Timer finished about <b>%d second</b> ago',
                                       'Timer finished about <b>%d seconds</b> ago',
                                       elapsed_time.seconds) % elapsed_time.seconds
                else:
                    minutes = elapsed_time.seconds / 60
                    message = ngettext('Timer finished about <b>%d minute</b> ago',
                                       'Timer finished about <b>%d minutes</b> ago',
                                       minutes) % minutes
                return message
            
            # TODO:
            # FIXME:
            # Reason for using a Python thread:
            #  To do all the procedures after timer has ended.  If I don't do
            #  this then after the timer ended and it had an auto-start and next
            #  timer defined, it would directly switch without any notification.
            #  Trying time.sleep() doesn't work as expected; it correctly starts
            #  the next timer, but it doesn't show the notification and the
            #  rest.
            class MyThread(threading.Thread):
                def __init__(self, timer_instance):
                    threading.Thread.__init__(self)
                    self.timer = timer_instance

                def run(self):
                    print "Starting thread..."
                    print "Calling popup notification.",
                    self.timer._call_notify(summary, message, reminder_message_func)
                    print "Starting pulsing button.", 
                    self.timer._start_pulsing_button()
                    print "Playing notification sound.", 
                    self.timer._play_notification_sound()
                    print "Running custom command.", 
                    self.timer._run_custom_command(command)

                    print "Ending Thread..."
            thread = MyThread(self)
            thread.start()
            thread.join()

            next_timer = self._timer.get_next_timer()
            auto_start = self._timer.get_auto_start()
            if auto_start and next_timer:
                # Start next timer
                self._stop_sound()
                self._call_notify(show=False)
                self._stop_pulsing_button()
                self._start_next_timer()
            elif not(auto_start) and next_timer:
                self._status_button.props.sensitive = False
                dialog_result = self._start_next_timer_dialog.get_response()
                self._status_button.props.sensitive = True
                if dialog_result:
                    # Start next timer
                    self._stop_sound()
                    self._call_notify(show=False)
                    self._stop_pulsing_button()
                    self._start_next_timer()
        else:
            self._stop_sound()
            self._call_notify(show=False)
            self._stop_pulsing_button()
        
        print "Updating status button..."
        self._update_status_button()
        print "Updating popup menu..."
        self._update_popup_menu()
    
    ## StatusButton callbacks ##
    
    def _on_status_button_clicked(self, button, data=None):
        current_state = self._timer.get_state()
        if current_state == core.Timer.STATE_IDLE:
            self._start_timer_dialog.show()
        elif current_state == core.Timer.STATE_FINISHED:
            self._timer.reset()
        elif current_state == core.Timer.STATE_PAUSED:
            # Temporarily disable status button while the Continue dialog is open.
            self._status_button.props.sensitive = False
            dialog_result = self._continue_dialog.get_response()
            self._status_button.props.sensitive = True
            if dialog_result == ui.ContinueTimerDialog.CONTINUE_TIMER:
                self._timer.start()
            elif dialog_result == ui.ContinueTimerDialog.STOP_TIMER:
                self._timer.reset()
            elif dialog_result == ui.ContinueTimerDialog.KEEP_PAUSED:
                pass
            else:
                assert False
        elif current_state == core.Timer.STATE_RUNNING:
            self._timer.stop()
    
    ## StartTimerDialog callbacks ##
    
    def _on_start_dialog_clicked_start(self, sender, data=None):
        (name, hours, minutes, seconds, command, next_timer, auto_start) = \
            self._start_timer_dialog.get_control_data()
        self._start_timer_with_settings(name, hours, minutes, seconds, command,
                                       next_timer, auto_start)
    
    def _on_start_dialog_clicked_manage_presets(self, sender, data=None):
        self._manage_presets_dialog.show()
    
    def _on_start_dialog_clicked_save(self, sender, name,
                                      hours, minutes, seconds, command,
                                      next_timer, auto_start, data=None):
        self._presets_store.add_preset(name, hours, minutes, seconds, command,
                                       next_timer, auto_start)
       
    def _on_start_dialog_clicked_preset(self, sender, row_path, data=None):
        row_iter = self._presets_store.get_model().get_iter(row_path)
        (name, hours, minutes, seconds, command, next_timer, auto_start) = \
               self._presets_store.get_preset(row_iter)
        self._start_timer_dialog.set_name_and_duration(name, hours, minutes,
                                                       seconds, command,
                                                       next_timer, auto_start)

    def _on_start_dialog_double_clicked_preset(self, sender, row_path, data=None):
        """Preset is double-clicked. Start the selected preset, and hide the
        dialog."""
        row_iter = self._presets_store.get_model().get_iter(row_path)
        (name, hours, minutes, seconds, command, next_timer, auto_start) = \
               self._presets_store.get_preset(row_iter)
        self._start_timer_with_settings(name, hours, minutes, seconds, command,
                                       next_timer, auto_start)
        self._start_timer_dialog.hide()

    ## Private methods ##
    
    def _start_timer_with_settings(self, name, hours, minutes, seconds,
                                   command, next_timer, auto_start):
        print "Resetting timer"
        if self._timer.get_state() != core.Timer.STATE_IDLE:
            self._timer.reset()
        self._timer.set_duration(utils.hms_to_seconds(hours, minutes, seconds))
        self._timer.set_name(name)
        self._timer.set_command(command)
        self._timer.set_next_timer(next_timer)
        self._timer.set_auto_start(auto_start)
        self._timer.start()
        
    def _restart_timer(self):
        self._timer.reset()
        self._timer.start()

    def _start_next_timer(self):
        """Start next timer, if defined."""
        next_timer = self._timer.get_next_timer()
        for row in self._presets_store.get_model():
            #print dir(row)
            if str(row[0]) == next_timer:
               (name, hours, minutes, seconds, command, next_timer, auto_start) = \
                    self._presets_store.get_preset(row.iter)
               break
        print "Starting timer with settings: ",
        print (name, hours, minutes, seconds, command, next_timer, auto_start)
        self._start_timer_with_settings(name, hours, minutes, seconds, command,
                                        next_timer, auto_start)


    def _play_notification_sound(self):
        if not self._mateconf.get_bool(TimerApplet._PLAY_SOUND_KEY):
            return
            
        sound_path = config.DEFAULT_SOUND_PATH
        if self._mateconf.get_bool(TimerApplet._USE_CUSTOM_SOUND_KEY):
            sound_path = self._mateconf.get_string(TimerApplet._CUSTOM_SOUND_PATH_KEY)
            
        print 'Playing notification sound: "%s"' % str(sound_path)
        self._play_sound(sound_path)
        print 'Started playing notification sound.'

    def _play_sound(self, file_path):
        if not file_path:
            print 'Invalid path to sound file'
            return
        self._gst_playbin.set_state(gst.STATE_NULL)
        sound_uri = 'file://' + file_path
        print 'Using GStreamer to play: ' + sound_uri
        self._gst_playbin.set_property('uri', sound_uri)
        self._gst_playbin.set_state(gst.STATE_PLAYING)

    def _run_custom_command(self, command):
        if command:
            print "Running custom command: " + command
            try:
                subprocess.call(shlex.split(command))
            except OSError:
                print "... failed. Command not found."

    def _stop_sound(self):
        self._gst_playbin.set_state(gst.STATE_NULL)
    
    def _start_pulsing_button(self):
        if self._mateconf.get_bool(TimerApplet._SHOW_PULSING_ICON_KEY):
            self._status_button.start_pulsing()

    def _stop_pulsing_button(self):
        self._status_button.stop_pulsing()
        
    def _show_about_dialog(self):
        self._about_dialog.run()
        self._about_dialog.hide()

    def _call_notify(self, summary=None, message=None,
                     reminder_message_func=None, show=True):
        if self._mateconf.get_bool(TimerApplet._SHOW_POPUP_NOTIFICATION_KEY):
            if show:
                self._notifier.begin(summary, message, reminder_message_func)
            else:
                self._notifier.end()
