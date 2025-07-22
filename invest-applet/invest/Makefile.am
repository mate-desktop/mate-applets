NULL =

AM_CPPFLAGS =			\
	$(WARN_FLAGS)		\
	$(MATE_APPLETS4_CFLAGS)	\
	$(LIBSOUP_CFLAGS)	\
	$(JSON_GLIB_CFLAGS)	\
	-I$(srcdir)		\
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(NULL)

APPLET_SOURCES =	\
	invest-applet.c	\
	invest-applet-chart.c	\
	$(NULL)

APPLET_LIBS =			\
	$(MATE_APPLETS4_LIBS)	\
	$(LIBSOUP_LIBS)		\
	$(JSON_GLIB_LIBS)	\
	$(NULL)

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libinvest-applet.la
libinvest_applet_la_SOURCES = $(APPLET_SOURCES)
libinvest_applet_la_CFLAGS = $(AM_CFLAGS)
libinvest_applet_la_LDFLAGS = -module -avoid-version
libinvest_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = invest-applet
invest_applet_SOURCES = $(APPLET_SOURCES)
invest_applet_CFLAGS = $(AM_CFLAGS)
invest_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

-include $(top_srcdir)/git.mk