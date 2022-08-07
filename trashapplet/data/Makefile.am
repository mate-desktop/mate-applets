appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.TrashApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.TrashAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.TrashAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

EXTRA_DIST = \
	$(applet_in_files).in	\
	$(service_in_files) \
	trashapplet-empty-progress.ui \
	trashapplet-menu.xml \
	trashapplet-resources.gresource.xml

CLEANFILES = $(applet_DATA) $(applet_in_files) $(service_DATA)

-include $(top_srcdir)/git.mk
