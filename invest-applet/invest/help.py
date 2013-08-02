# -*- coding: utf-8 -*-

import gi
gi.require_version("Gtk", "2.0")
from gi.repository import Gtk
from gi.repository import Gdk

def show_help():
	Gtk.show_uri(None, "help:invest-applet", Gdk.CURRENT_TIME)

def show_help_section(id):
	Gtk.show_uri(None, "help:invest-applet?%s" % id, Gdk.CURRENT_TIME)
