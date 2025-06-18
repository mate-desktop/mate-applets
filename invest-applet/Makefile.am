SUBDIRS = invest data docs

# ******************************************************************************
# Panel applet configuration processing
# ******************************************************************************
appletdir = $(datadir)/mate-panel/applets
applet_in_files = data/org.mate.applets.InvestApplet.mate-panel-applet.in
applet_DATA = data/org.mate.applets.InvestApplet.mate-panel-applet

if ENABLE_IN_PROCESS
APPLET_LOCATION = $(pkglibdir)/libinvest-applet.so
else !ENABLE_IN_PROCESS
APPLET_LOCATION = $(libexecdir)/invest-applet
endif !ENABLE_IN_PROCESS

$(applet_in_files): data/org.mate.applets.InvestApplet.mate-panel-applet.in.in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
            -e "s|\@APPLET_IN_PROCESS\@|$(APPLET_IN_PROCESS)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

# ******************************************************************************
# D-Bus service processing (for out-of-process build)
# ******************************************************************************
if !ENABLE_IN_PROCESS
servicedir = $(datadir)/dbus-1/services
service_in_files = data/org.mate.panel.applet.InvestAppletFactory.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN)sed \
            -e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
            $< > $@
endif !ENABLE_IN_PROCESS

CLEANFILES = \
	$(applet_DATA) \
	$(applet_in_files) \
	$(service_DATA)

-include $(top_srcdir)/git.mk
