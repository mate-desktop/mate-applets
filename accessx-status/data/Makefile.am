NULL =

applet_in_files = org.mate.applets.AccessxStatusApplet.mate-panel-applet.desktop.in
service_in_files = org.mate.panel.applet.AccessxStatusAppletFactory.service.in

if ENABLE_IN_PROCESS
APPLET_LOCATION = $(pkglibdir)/libmate-accessx-status-applet.so
else !ENABLE_IN_PROCESS
APPLET_LOCATION = $(libexecdir)/accessx-status-applet
endif !ENABLE_IN_PROCESS

appletdir       = $(datadir)/mate-panel/applets
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@APPLET_IN_PROCESS\@|$(APPLET_IN_PROCESS)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

if !ENABLE_IN_PROCESS
servicedir       = $(datadir)/dbus-1/services
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@
endif !ENABLE_IN_PROCESS

CLEANFILES = \
	$(applet_DATA) \
	$(applet_in_files) \
	$(service_DATA) \
	$(NULL)

EXTRA_DIST = \
	$(applet_in_files).in \
	$(service_in_files) \
	accessx-status-applet-menu.xml \
	accessx-status-resources.gresource.xml \
	$(NULL)


-include $(top_srcdir)/git.mk
