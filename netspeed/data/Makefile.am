NULL =

applet_in_files = org.mate.applets.NetspeedApplet.mate-panel-applet.desktop.in
service_in_files = org.mate.panel.applet.NetspeedAppletFactory.service.in
gschema_in_files = org.mate.panel.applet.netspeed.gschema.xml.in

if ENABLE_IN_PROCESS
APPLET_LOCATION = $(pkglibdir)/libmate-netspeed-applet.so
else !ENABLE_IN_PROCESS
APPLET_LOCATION = $(libexecdir)/mate-netspeed-applet
endif !ENABLE_IN_PROCESS

appletdir       = $(datadir)/mate-panel/applets
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@APPLET_IN_PROCESS\@|$(APPLET_IN_PROCESS)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

if !ENABLE_IN_PROCESS
servicedir = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@
endif !ENABLE_IN_PROCESS

gsettings_SCHEMAS = $(gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(applet_in_files).in	\
	$(service_in_files) \
	$(gschema_in_files) \
	netspeed-details.ui \
	netspeed-menu.xml \
	netspeed-preferences.ui \
	netspeed-resources.gresource.xml \
	$(NULL)

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	$(service_DATA) \
	$(applet_in_files) \
	$(applet_DATA)


-include $(top_srcdir)/git.mk
