# -*- coding: utf-8 -*-
from mate_invest.defs import GTK_API_VERSION

import gi
gi.require_version("Gtk", GTK_API_VERSION)
from gi.repository import Gtk
from gi.repository import Gdk

def show_help():
	Gtk.show_uri(None, "help:mate-invest-applet", Gdk.CURRENT_TIME)

def show_help_section(id):
	Gtk.show_uri(None, "help:mate-invest-applet/%s" % id, Gdk.CURRENT_TIME)
