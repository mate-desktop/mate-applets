SUBDIRS = sensors-applet lib plugins pixmaps help

if ENABLE_IN_PROCESS
APPLET_LOCATION = $(libdir)/mate-sensors-applet/libmate-sensors-applet.so
else
APPLET_LOCATION = $(libexecdir)/mate-sensors-applet

servicedir = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.SensorsAppletFactory.service.in
service_DATA = $(service_in_files:.service.in=.service)

org.mate.panel.applet.SensorsAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
		-e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
		$< > $@

endif

appletdir =  $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.SensorsApplet.mate-panel-applet.desktop.in
applet_DATA = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@APPLET_IN_PROCESS\@|$(APPLET_IN_PROCESS)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

uidir 		= $(datadir)/mate-sensors-applet/ui
ui_DATA 	= SensorsApplet.xml

gsettingsschema_in_files = \
	org.mate.sensors-applet.gschema.xml.in \
	org.mate.sensors-applet.sensor.gschema.xml.in

gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

CLEANFILES = $(applet_DATA) \
	$(applet_in_files) \
	$(service_DATA) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid

EXTRA_DIST = $(ui_DATA) \
	$(applet_in_files).in \
	$(service_in_files) \
	$(gsettingsschema_in_files)

DISTCLEANFILES	=

-include $(top_srcdir)/git.mk
