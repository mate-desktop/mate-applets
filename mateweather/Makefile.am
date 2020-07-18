## Process this file with automake to produce Makefile.in

SUBDIRS = docs

AM_CPPFLAGS =					\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	-DWEATHER_RESOURCE_PATH=\""/org/mate/mate-applets/mateweather/"\" \
	$(MATE_APPLETS4_CFLAGS)			\
	$(LIBNOTIFY_CFLAGS)			\
	$(LIBMATEWEATHER_CFLAGS)		\
	${WARN_CFLAGS}

libexec_PROGRAMS = mateweather-applet

BUILT_SOURCES = mateweather-resources.c mateweather-resources.h
nodist_mateweather_applet_SOURCES = $(BUILT_SOURCES)
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
	$(MATE_LIBS2_LIBS)	\
	$(LIBMATEWEATHER_LIBS)

mateweather-resources.c: mateweather-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/mateweather-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name mateweather $<

mateweather-resources.h: mateweather-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/mateweather-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name mateweather $<

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.MateWeatherApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.MateWeatherAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.MateWeatherAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES =			\
	$(applet_DATA)		\
	$(applet_in_files)	\
	$(service_DATA)		\
	$(BUILT_SOURCES)

EXTRA_DIST =					\
	mateweather-applet-menu.xml		\
	mateweather-dialog.ui			\
	mateweather-resources.gresource.xml	\
	$(applet_in_files).in			\
	$(service_in_files)			\
	$(ui_DATA)

-include $(top_srcdir)/git.mk
