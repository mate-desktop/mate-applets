multiload_gschema_in_files = org.mate.panel.applet.multiload.gschema.xml.in
gsettings_SCHEMAS = $(multiload_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

appletdir          = $(datadir)/mate-panel/applets
applet_in_in_files = org.mate.applets.MultiLoadApplet.mate-panel-applet.desktop.in.in
applet_in_files    = $(applet_in_in_files:.desktop.in.in=.desktop.in)
applet_DATA        = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir          = $(datadir)/dbus-1/services
service_in_files    = org.mate.panel.applet.MultiLoadAppletFactory.service.in
service_DATA        = $(service_in_files:.service.in=.service)

org.mate.panel.applet.MultiLoadAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = \
	$(applet_DATA) \
	$(applet_in_files) \
	$(service_DATA) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid

EXTRA_DIST = \
	$(applet_in_in_files) \
	$(service_in_files) \
	$(multiload_gschema_in_files) \
	multiload-applet-menu.xml \
	multiload-resources.gresource.xml \
	properties.ui

-include $(top_srcdir)/git.mk
