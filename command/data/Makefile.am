applets_in_files = org.mate.applets.CommandApplet.mate-panel-applet.desktop.in
service_in_files = org.mate.panel.applet.CommandAppletFactory.service.in
gschema_in_files = org.mate.panel.applet.command.gschema.xml.in

if ENABLE_IN_PROCESS
APPLET_LOCATION = $(pkglibdir)/libcommand-applet.so
else !ENABLE_IN_PROCESS
APPLET_LOCATION = $(libexecdir)/command-applet
endif !ENABLE_IN_PROCESS

appletsdir       = $(datadir)/mate-panel/applets
applets_DATA     = $(applets_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applets_in_files): $(applets_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@APPLET_IN_PROCESS\@|$(APPLET_IN_PROCESS)|" \
		$< > $@

$(applets_DATA): $(applets_in_files) Makefile
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
	$(applets_in_files).in \
	$(service_in_files) \
	$(gschema_in_files) \
	command-preferences.ui \
	command-resources.gresource.xml

CLEANFILES = \
	$(applets_DATA) \
	$(applets_in_files) \
	$(service_DATA) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid

-include $(top_srcdir)/git.mk
