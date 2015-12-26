APPLET_LOCATION = $(libexecdir)/mate-netspeed-applet

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.NetspeedApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@
%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

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
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST = \
	org.mate.applets.NetspeedApplet.mate-panel-applet.in.in \
	$(xmlui_DATA) \
	$(service_in_files) \
	$(netspeed_gschema_in_files)

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	$(service_DATA) \
	$(applet_in_files) \
	org.mate.applets.NetspeedApplet.mate-panel-applet

