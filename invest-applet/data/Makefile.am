SUBDIRS = art

# ******************************************************************************
# Panel applet DBUS stuff
# ******************************************************************************
servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.InvestAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.InvestAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

# ******************************************************************************
# Panel stuff
# ******************************************************************************

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.InvestApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

# ******************************************************************************
# Misc data
# ******************************************************************************

uidir = $(datadir)/mate-applets
ui_DATA = \
	Invest_Applet.xml
builder_DATA = \
	prefs-dialog.ui \
	financialchart.ui

investbindir = $(libdir)/invest-applet

# ******************************************************************************
# Build rules
# ******************************************************************************

CLEANFILES = \
	$(applet_DATA) \
	$(applet_DATA).in \
	$(service_in_files:.service.in=.service)



EXTRA_DIST = \
	org.mate.applets.InvestApplet.mate-panel-applet.in.in \
	$(service_in_files) \
	$(ui_DATA) \
	$(builder_DATA)

-include $(top_srcdir)/git.mk
