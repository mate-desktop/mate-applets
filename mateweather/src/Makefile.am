NULL =

AM_CPPFLAGS =					\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	-DWEATHER_RESOURCE_PATH=\""/org/mate/mate-applets/mateweather/"\" \
	$(MATE_APPLETS4_CFLAGS)			\
	$(LIBNOTIFY_CFLAGS)			\
	$(LIBMATEWEATHER_CFLAGS)		\
	${WARN_CFLAGS}

BUILT_SOURCES =			\
	mateweather-resources.c	\
	mateweather-resources.h	\
	$(NULL)
APPLET_SOURCES =					\
	mateweather.h					\
	main.c						\
	mateweather-about.c mateweather-about.h		\
	mateweather-pref.c mateweather-pref.h		\
	mateweather-dialog.c mateweather-dialog.h	\
	mateweather-applet.c mateweather-applet.h	\
	$(NULL)

APPLET_LIBS =			\
	$(LIBNOTIFY_LIBS) 	\
	$(MATE_APPLETS4_LIBS)	\
	$(MATE_LIBS2_LIBS)	\
	$(LIBMATEWEATHER_LIBS)	\
	$(NULL)

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmateweather-applet.la
nodist_libmateweather_applet_la_SOURCES = $(BUILT_SOURCES)
libmateweather_applet_la_SOURCES = $(APPLET_SOURCES)
libmateweather_applet_la_CFLAGS = $(AM_CFLAGS)
libmateweather_applet_la_LDFLAGS = -module -avoid-version
libmateweather_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = mateweather-applet
nodist_mateweather_applet_SOURCES = $(BUILT_SOURCES)
mateweather_applet_SOURCES = $(APPLET_SOURCES)
mateweather_applet_CFLAGS = $(AM_CFLAGS)
mateweather_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

mateweather-resources.c: ../data/mateweather-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/mateweather-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name mateweather $<

mateweather-resources.h: ../data/mateweather-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/mateweather-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name mateweather $<

CLEANFILES =			\
	$(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
