NULL =

mate_netspeed_applet_libdir= $(pkglibdir)
mate_netspeed_applet_lib_LTLIBRARIES=libmate-netspeed-applet.la

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

libmate_netspeed_applet_la_SOURCES =	\
	backend.h		\
	backend.c		\
	netspeed.c		\
	netspeed.h		\
	netspeed-preferences.c	\
	netspeed-preferences.h	\
	netspeed-resources.c   \
	netspeed-resources.h   \
	$(NULL)

if HAVE_NL
libmate_netspeed_applet_la_SOURCES +=	\
	nl80211.h		\
	ieee80211.h		\
	$(NULL)
endif

libmate_netspeed_applet_la_LIBADD = $(GIO_LIBS) $(GTOP_APPLETS_LIBS) \
       $(MATE_APPLETS4_LIBS) \
       $(INTLLIBS) -lm

if HAVE_IW
libmate_netspeed_applet_la_LIBADD += $(IW_LIBS)
endif
if HAVE_NL
libmate_netspeed_applet_la_LIBADD += $(NL_LIBS)
endif

netspeed-resources.c: $(srcdir)/../data/netspeed-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/netspeed-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name netspeed $<

netspeed-resources.h: $(srcdir)/../data/netspeed-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/netspeed-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name netspeed $<

CLEANFILES =			\
	$(NULL)

-include $(top_srcdir)/git.mk
