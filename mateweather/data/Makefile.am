APPLET_LOCATION = $(libdir)/mate-applets/libmateweather-applet.so

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.MateWeatherApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@



CLEANFILES =			\
	$(applet_DATA)		\
	$(applet_in_files)

EXTRA_DIST =					\
	mateweather-applet-menu.xml		\
	mateweather-dialog.ui			\
	mateweather-resources.gresource.xml	\
	$(applet_in_files:=.in)			\
	$(ui_DATA)

-include $(top_srcdir)/git.mk
