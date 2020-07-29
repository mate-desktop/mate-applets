NULL =

APPLET_LOCATION = $(libexecdir)/mate-netspeed-applet

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.NetspeedApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

xmluidir   = $(datadir)/mate/ui
xmlui_DATA = netspeed-menu.xml

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.NetspeedAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.NetspeedAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
		-e "s|\@LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@

netspeed_gschema_in_files = org.mate.panel.applet.netspeed.gschema.xml.in
gsettings_SCHEMAS = $(netspeed_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(applet_in_files).in	\
	$(xmlui_DATA) \
	$(service_in_files) \
	$(netspeed_gschema_in_files) \
	netspeed-details.ui \
	netspeed-preferences.ui \
	netspeed-resources.gresource.xml \
	$(NULL)

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	$(service_DATA) \
	$(applet_in_files) \
	$(applet_DATA)


-include $(top_srcdir)/git.mk
