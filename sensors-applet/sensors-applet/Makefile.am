## Process this file with automake to produce Makefile.in

uidir = $(datadir)/mate-sensors-applet/ui

AM_CPPFLAGS = -DMATELOCALEDIR=\""$(datadir)/locale/"\" \
	-DG_LOG_DOMAIN=\""sensors-applet"\" \
	-DPIXMAPS_DIR=\""$(datadir)/pixmaps/$(PACKAGE)/"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DUIDIR=\""$(pkgdatadir)/ui"\" \
	-DSENSORS_APPLET_PLUGIN_DIR=\""$(libdir)/$(PACKAGE)/plugins"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
	-I$(top_srcdir)/sensors-applet \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(MATE_CFLAGS) \
	$(CAIRO_CFLAGS) \
	$(LIBNOTIFY_CFLAGS) \
	$(WARN_CFLAGS)

mate_sensors_applet_libdir= $(pkglibdir)

LIBS = $(GLIB_LIBS) $(GTK_LIBS) $(MATE_LIBS) $(CAIRO_LIBS) $(LIBNOTIFY_LIBS)

if LIBNOTIFY
libnotify_SRC = active-sensor-libnotify.c \
		active-sensor-libnotify.h
else
libnotify_SRC =
endif

if ENABLE_IN_PROCESS
mate_sensors_applet_lib_LTLIBRARIES=libmate-sensors-applet.la
libmate_sensors_applet_la_CFLAGS = \
	$(MATE_APPLETS4_CFLAGS)		\
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

libmate_sensors_applet_la_LDFLAGS = \
	-module -avoid-version \
	$(WARN_LDFLAGS) \
	$(AM_LDFLAGS) \
	$(NULL)

libmate_sensors_applet_la_LIBADD = \
	$(MATE_APPLETS4_LIBS)		\
	$(NULL)

libmate_sensors_applet_la_SOURCES = main.c \
		about-dialog.c \
		about-dialog.h \
		active-sensor.c \
		active-sensor.h \
		prefs-dialog.c \
		prefs-dialog.h \
		sensor-config-dialog.c \
		sensor-config-dialog.h \
		sensors-applet.c \
		sensors-applet.h \
		sensors-applet-sensor.h \
		sensors-applet-plugins.c \
		sensors-applet-plugins.h \
		sensors-applet-settings.c \
		sensors-applet-settings.h \
		$(libnotify_SRC)
else
libexec_PROGRAMS = mate-sensors-applet
mate_sensors_applet_SOURCES = main.c \
		about-dialog.c \
		about-dialog.h \
		active-sensor.c \
		active-sensor.h \
		prefs-dialog.c \
		prefs-dialog.h \
		sensor-config-dialog.c \
		sensor-config-dialog.h \
		sensors-applet.c \
		sensors-applet.h \
		sensors-applet-sensor.h \
		sensors-applet-plugins.c \
		sensors-applet-plugins.h \
		sensors-applet-settings.c \
		sensors-applet-settings.h \
		$(libnotify_SRC)
endif

# install headers for plugins to use
INST_H_FILES = sensors-applet-plugin.h sensors-applet-sensor.h

headerdir = $(prefix)/include/mate-sensors-applet

header_DATA = $(INST_H_FILES)

# since it is separate
EXTRA_DIST = sensors-applet-plugin.h

-include $(top_srcdir)/git.mk
