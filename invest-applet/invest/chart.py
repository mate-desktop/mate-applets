#!/usr/bin/env python
from mate_invest.defs import GTK_API_VERSION

import gi
gi.require_version("Gtk", GTK_API_VERSION)
from gi.repository import Gtk
from gi.repository import Gdk
from gi.repository import GdkPixbuf
from gi.repository import GObject
import os
import mate_invest
from gettext import gettext as _
from mate_invest import *
import sys
from os.path import join
import urllib
from threading import Thread
import time

AUTOREFRESH_TIMEOUT = 20*60*1000 # 15 minutes

# based on http://www.johnstowers.co.nz/blog/index.php/2007/03/12/threading-and-pygtk/
class _IdleObject(GObject.GObject):
	"""
	Override GObject.GObject to always emit signals in the main thread
	by emmitting on an idle handler
	"""
	def __init__(self):
		GObject.GObject.__init__(self)

	def emit(self, *args):
		GObject.idle_add(GObject.GObject.emit,self,*args)

class ImageRetriever(Thread, _IdleObject):
	"""
	Thread which uses gobject signals to return information
	to the GUI.
	"""
	__gsignals__ =  { 
			"completed": (
				GObject.SignalFlags.RUN_LAST, None, []),
			# FIXME: should we be making use of this?
			#"progress": (
			#	GObject.SignalFlags.RUN_LAST, None, [
			#	GObject.TYPE_FLOAT])        #percent complete
			}

	def __init__(self, image_url):
		Thread.__init__(self)
		_IdleObject.__init__(self)	
		self.image_url = image_url
		self.retrieved = False
		
	def run(self):
		self.image = Gtk.Image()
		try: sock = urllib.urlopen(self.image_url, proxies = mate_invest.PROXY)
		except Exception, msg:
			mate_invest.debug("Error while opening %s: %s" % (self.image_url, msg))
		else:
			loader = GdkPixbuf.PixbufLoader()
			loader.connect("closed", lambda loader: self.image.set_from_pixbuf(loader.get_pixbuf()))
			loader.write(sock.read())
			sock.close()
			loader.close()
			self.retrieved = True
		self.emit("completed")

# p:
#  eX = Exponential Moving Average
#  mX = Moving Average
#  b = Bollinger Bands Overlay
#  v = Volume Overlay
#  p = Parabolic SAR overlay
#  s = Splits Overlay
# q:
#  l = Line
#  c = Candles
#  b = Bars
# l:
#  on = Logarithmic
#  off = Linear
# z:
#  l = Large
#  m = Medium
# t:
#  Xd = X Days
#  Xm = X Months
#  Xy = X Years
# a:
#  fX = MFI X days
#  ss = Slow Stochastic
#  fs = Fast Stochastic
#  wX = W%R X Days
#  mX-Y-Z = MACD X Days, Y Days, Signal
#  pX = ROC X Days
#  rX = RSI X Days
#  v = Volume
#  vm = Volume +MA
# c:
#  X = compare with X
#

class FinancialChart:
	def __init__(self, ui):
		self.ui = ui
		
		#Time ranges of the plot (parameter / combo-box t)
		self.time_ranges = ["1d", "5d", "3m", "6m", "1y", "5y", "my"]
		
		#plot types (parameter / combo-box q)
		self.plot_types = ["l", "b", "c"]

		#plot scales (parameter / combo-box l)
		self.plot_scales = ["off", "on"]

		# Window Properties
		win = ui.get_object("window")
		win.set_title(_("Financial Chart"))
		
		try:
			pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_size(join(mate_invest.ART_DATA_DIR, "invest_neutral.svg"), 96,96)
			self.ui.get_object("plot").set_from_pixbuf(pixbuf)
		except Exception, msg:
			mate_invest.debug("Could not load 'invest-neutral.svg' file: %s" % msg)
			pass
		
		# Defaut comboboxes values
		for widget in ["t", "q", "l"]:
			ui.get_object(widget).set_active(0)
		
		# Connect every option widget to its corresponding change signal	
		symbolentry = ui.get_object("s")
		refresh_chart_callback = lambda w: self.on_refresh_chart()
		
		for widgets, signal in [
			(("pm5","pm10","pm20","pm50","pm100","pm200",
			"pe5","pe10", "pe20","pe50","pe100","pe200",
			"pb","pp","ps","pv",
			"ar","af","ap","aw","am","ass","afs","av","avm"), "toggled"),
			(("t", "q", "l"), "changed"),
			(("s",), "activate"),
		]:
			for widget in widgets:
				ui.get_object(widget).connect(signal, refresh_chart_callback)
		
		ui.get_object("progress").hide()
		
		# Connect auto-refresh widget
		self.autorefresh_id = 0
		ui.get_object("autorefresh").connect("toggled", self.on_autorefresh_toggled)
										
	def on_refresh_chart(self, from_timer=False):
		tickers = self.ui.get_object("s").get_text()
			
		if tickers.strip() == "":
			return True
		
		# FIXME: We don't just do US stocks, so we can't be this
		# simplistic about it, but it is a good idea.
		#if from_timer and not ustime.hour_between(9, 16):
		#	return True
			
		tickers = [ticker.strip().upper() for ticker in tickers.split(' ') if ticker != ""]
		
		# Update Window Title ------------------------------------------------------
		win = self.ui.get_object("window")
		title = _("Financial Chart - %s")
		titletail = ""
		for ticker in tickers:
			titletail += "%s / " % ticker
		title = title % titletail
		
		win.set_title(title[:-3])

		# Detect Comparison or simple chart ----------------------------------------
		opt = ""
		for ticker in tickers[1:]:
			opt += "&c=%s" % ticker
		
		# Create the overlay string ------------------------------------------------
		p = ""
		for name, param in [
			("pm5",   5),
			("pm10",  10),
			("pm20",  20),
			("pm50",  50),
			("pm100", 100),
			("pm200", 200),
			("pe5",   5),
			("pe10",  10),
			("pe20",  20),
			("pe50",  50),
			("pe100", 100),
			("pe200", 200),
			("pb", ""),
			("pp", ""),
			("ps", ""),
			("pv", ""),
		]:
			if self.ui.get_object(name).get_active():
				p += "%s%s," % (name[1], param)
			
		# Create the indicators string ---------------------------------------------
		a = ""
		for name, param in [
			("ar",  14),
			("af",  14),
			("ap",  12),
			("aw",  14),
			("am",  "26-12-9"),
			("ass", ""),
			("afs", ""),
			("av",  ""),
			("avm", ""),
		]:
			if self.ui.get_object(name).get_active():
				a += "%s%s," % (name[1:], param)
		
		# Create the image URL -----------------------------------------------------
		chart_base_url = "http://chart.finance.yahoo.com/z?s=%(s)s&t=%(t)s&q=%(q)s&l=%(l)s&z=%(z)s&p=%(p)s&a=%(a)s%(opt)s"
		url = chart_base_url % {
			"s": tickers[0],
			"t": self.time_ranges[self.ui.get_object("t").get_active()],
			"q": self.plot_types[self.ui.get_object("q").get_active()],
			"l": self.plot_scales[self.ui.get_object("l").get_active()],
			"z": "l",
			"p": p,
			"a": a,
			"opt": opt,
		}

		# Download and display the image -------------------------------------------	
		progress = self.ui.get_object("progress")
		progress.set_text(_("Opening Chart"))
		progress.show()
		
		image_retriever = ImageRetriever(url)
		image_retriever.connect("completed", self.on_retriever_completed)
		image_retriever.start()

		# Update timer if needed
		self.on_autorefresh_toggled(self.ui.get_object("autorefresh"))
		return True
	
	def on_retriever_completed(self, retriever):
		self.ui.get_object("plot").set_from_pixbuf(retriever.image.get_pixbuf())
		progress = self.ui.get_object("progress")
		if retriever.retrieved == True:
			progress.set_text(_("Chart downloaded"))
		else:
			progress.set_text(_("Chart could not be downloaded"))
	
	def on_autorefresh_toggled(self, autorefresh):
		if self.autorefresh_id != 0:
			GObject.source_remove(self.autorefresh_id)
			self.autorefresh_id = 0
			
		if autorefresh.get_active():
			self.autorefresh_id = GObject.timeout_add(AUTOREFRESH_TIMEOUT, self.on_refresh_chart, True)

def show_chart(tickers):
	ui = Gtk.Builder();
	ui.add_from_file(os.path.join(mate_invest.BUILDER_DATA_DIR, "financialchart.ui"))
	chart = FinancialChart(ui)
	ui.get_object("s").set_text(' '.join(tickers))
	chart.on_refresh_chart()
	return ui.get_object("window")

