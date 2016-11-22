#!/usr/bin/env python
import mate_invest.defs

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk
from gi.repository import GObject
from gi.repository import MatePanelApplet

import getopt, sys
from os.path import *

# Allow to use uninstalled
def _check(path):
	return exists(path) and isdir(path) and isfile(path+"/Makefile.am")

name = join(dirname(__file__), '..')
if _check(name):
	print 'Running uninstalled invest, modifying PYTHONPATH'
	sys.path.insert(0, abspath(name))
else:
	sys.path.insert(0, abspath("@PYTHONDIR@"))

# Now the path is set, import our applet
import mate_invest, mate_invest.applet, mate_invest.help

# Prepare i18n
import gettext, locale
gettext.bindtextdomain(mate_invest.defs.GETTEXT_PACKAGE, mate_invest.defs.MATELOCALEDIR)
gettext.textdomain(mate_invest.defs.GETTEXT_PACKAGE)
locale.bindtextdomain(mate_invest.defs.GETTEXT_PACKAGE, mate_invest.defs.MATELOCALEDIR)
locale.textdomain(mate_invest.defs.GETTEXT_PACKAGE)

from gettext import gettext as _

def applet_factory(applet, iid, data):
	mate_invest.debug('Starting invest instance: %s %s'% ( applet, iid ))
	mate_invest.applet.InvestApplet(applet)
	return True

# Return a standalone window that holds the applet
def build_window():
	app = Gtk.Window(Gtk.WindowType.TOPLEVEL)
	app.set_title(_("Invest Applet"))
	app.connect("destroy", Gtk.main_quit)
	app.set_property('resizable', False)

	applet = MatePanelApplet.Applet()
	applet_factory(applet, None, None)
	applet.reparent(app)

	app.show_all()

	return app


def usage():
	print """=== Invest applet: Usage
$ invest-applet [OPTIONS]

OPTIONS:
	-h, --help			Print this help notice.
	-d, --debug			Enable debug output (default=off).
	-w, --window		Launch the applet in a standalone window for test purposes (default=no).
	"""
	sys.exit()

if __name__ == "__main__":
	standalone = False

	try:
		opts, args = getopt.getopt(sys.argv[1:], "hdw", ["help", "debug", "window"])
	except getopt.GetoptError:
		# Unknown args were passed, we fallback to behave as if
		# no options were passed
		opts = []
		args = sys.argv[1:]

	for o, a in opts:
		if o in ("-h", "--help"):
			usage()
		elif o in ("-d", "--debug"):
			mate_invest.DEBUGGING = True
			mate_invest.debug("Debugging enabled")
			# these messages cannot be turned by mate_invest.DEBUGGING at their originating location,
			# because that variable was set here to be True
			mate_invest.debug("Data Dir: %s" % mate_invest.SHARED_DATA_DIR)
			mate_invest.debug("Detected PROXY: %s" % mate_invest.PROXY)
		elif o in ("-w", "--window"):
			standalone = True

	if standalone:
		build_window()
		Gtk.main()
	else:
		MatePanelApplet.Applet.factory_main(
			"InvestAppletFactory",
			True,
			MatePanelApplet.Applet.__gtype__,
			applet_factory,
			None)
