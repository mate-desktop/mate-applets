#!/usr/bin/env python
#

import gobject
import gtk, mateapplet

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
import mate-invest, mate-invest.applet, mate-invest.defs, mate-invest.help

# Prepare i18n
import gettext, locale
gettext.bindtextdomain(mate-invest.defs.GETTEXT_PACKAGE, mate-invest.defs.MATELOCALEDIR)
gettext.textdomain(mate-invest.defs.GETTEXT_PACKAGE)
locale.bindtextdomain(mate-invest.defs.GETTEXT_PACKAGE, mate-invest.defs.MATELOCALEDIR)
locale.textdomain(mate-invest.defs.GETTEXT_PACKAGE)

from gettext import gettext as _

def applet_factory(applet, iid):
	mate-invest.debug('Starting invest instance: %s %s'% ( applet, iid ))
	mate-invest.applet.InvestApplet(applet)
	return True

# Return a standalone window that holds the applet
def build_window():
	app = gtk.Window(gtk.WINDOW_TOPLEVEL)
	app.set_title(_("Invest Applet"))
	app.connect("destroy", gtk.main_quit)
	app.set_property('resizable', False)

	applet = mateapplet.Applet()
	applet_factory(applet, None)
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
		# Unknown args were passed, we fallback to bahave as if
		# no options were passed
		opts = []
		args = sys.argv[1:]

	for o, a in opts:
		if o in ("-h", "--help"):
			usage()
		elif o in ("-d", "--debug"):
			mate-invest.DEBUGGING = True
			mate-invest.debug("Debugging enabled")
			# these messages cannot be turned by mate-invest.DEBUGGING at their originating location,
			# because that variable was set here to be True
			mate-invest.debug("Data Dir: %s" % mate-invest.SHARED_DATA_DIR)
			mate-invest.debug("Detected PROXY: %s" % mate-invest.PROXY)
		elif o in ("-w", "--window"):
			standalone = True

	if standalone:
		build_window()
		gtk.main()
	else:
		mateapplet.matecomponent_factory(
			"OAFIID:Invest_Applet_Factory",
			mateapplet.Applet.__gtype__,
			mate-invest.defs.PACKAGE,
			mate-invest.defs.VERSION,
			applet_factory)
