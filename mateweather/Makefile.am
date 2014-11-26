## Process this file with automake to produce Makefile.in

SUBDIRS = docs

AM_CPPFLAGS =					\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	-DMATEWEATHER_MENU_UI_DIR=\""$(uidir)"\"	\
	$(MATE_APPLETS4_CFLAGS)		\
	$(MATEDESKTOP_CFLAGS)		\
	$(LIBNOTIFY_CFLAGS)			\
	$(LIBMATEWEATHER_CFLAGS)			\
	$(DBUS_CFLAGS)

libexec_PROGRAMS = mateweather-applet

mateweather_applet_SOURCES = \
	mateweather.h \
	main.c \
	mateweather-about.c mateweather-about.h \
	mateweather-pref.c mateweather-pref.h \
	mateweather-dialog.c mateweather-dialog.h \
	mateweather-applet.c mateweather-applet.h

mateweather_applet_LDADD = 	\
	$(LIBNOTIFY_LIBS) 	\
	$(MATE_APPLETS4_LIBS)	\
	$(MATEDESKTOP_LIBS)	\
	$(MATE_LIBS2_LIBS)	\
	$(LIBMATEWEATHER_LIBS)	\
	$(DBUS_LIBS)

uidir = $(datadir)/mate/ui
ui_DATA = mateweather-applet-menu.xml

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.MateWeatherApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.MateWeatherAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.MateWeatherAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA)

EXTRA_DIST = \
	org.mate.applets.MateWeatherApplet.mate-panel-applet.in.in	\
	$(service_in_files)					\
	$(ui_DATA)

-include $(top_srcdir)/git.mk
