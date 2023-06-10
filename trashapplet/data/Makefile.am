APPLET_LOCATION = $(libdir)/mate-applets/libtrashapplet.so

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.TrashApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@


EXTRA_DIST = \
	$(applet_in_files).in	\
	trashapplet-empty-progress.ui \
	trashapplet-menu.xml \
	trashapplet-resources.gresource.xml

CLEANFILES = $(applet_DATA) $(applet_in_files)

-include $(top_srcdir)/git.mk
