NULL =

APPLET_LOCATION = $(libdir)/mate-applets/libmate-netspeed-applet.so

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.NetspeedApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

netspeed_gschema_in_files = org.mate.panel.applet.netspeed.gschema.xml.in
gsettings_SCHEMAS = $(netspeed_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(applet_in_files).in	\
	$(netspeed_gschema_in_files) \
	netspeed-details.ui \
	netspeed-menu.xml \
	netspeed-preferences.ui \
	netspeed-resources.gresource.xml \
	$(NULL)

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	$(applet_in_files) \
	$(applet_DATA)


-include $(top_srcdir)/git.mk
