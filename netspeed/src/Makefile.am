NULL =

AM_CPPFLAGS =			\
	-I$(top_srcdir)		\
	-I$(includedir)		\
	$(GIO_CFLAGS)		\
	$(GTOP_APPLETS_CFLAGS)	\
	$(MATE_APPLETS4_CFLAGS)	\
	-DNETSPEED_RESOURCE_PATH=\""/org/mate/mate-applets/netspeed/"\" \
	${WARN_CFLAGS}

if HAVE_NL
AM_CPPFLAGS += $(NL_CFLAGS)
endif

BUILT_SOURCES =			\
	netspeed-resources.c	\
	netspeed-resources.h	\
	$(NULL)
APPLET_SOURCES =		\
	backend.h		\
	backend.c		\
	netspeed.c		\
	netspeed.h		\
	netspeed-preferences.c	\
	netspeed-preferences.h	\
	$(NULL)

if HAVE_NL
APPLET_SOURCES +=		\
	nl80211.h		\
	ieee80211.h		\
	$(NULL)
endif

APPLET_LIBS = \
	$(GIO_LIBS) \
	$(GTOP_APPLETS_LIBS) \
	$(MATE_APPLETS4_LIBS) \
	$(INTLLIBS) -lm

if HAVE_IW
APPLET_LIBS += $(IW_LIBS)
endif
if HAVE_NL
APPLET_LIBS += $(NL_LIBS)
endif

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmate-netspeed-applet.la
nodist_libmate_netspeed_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_netspeed_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_netspeed_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_netspeed_applet_la_LDFLAGS = -module -avoid-version
libmate_netspeed_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = mate-netspeed-applet
nodist_mate_netspeed_applet_SOURCES = $(BUILT_SOURCES)
mate_netspeed_applet_SOURCES = $(APPLET_SOURCES)
mate_netspeed_applet_CFLAGS = $(AM_CFLAGS)
mate_netspeed_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

netspeed-resources.c: $(srcdir)/../data/netspeed-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/netspeed-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name netspeed $<

netspeed-resources.h: $(srcdir)/../data/netspeed-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/netspeed-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name netspeed $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
