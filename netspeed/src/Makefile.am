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

libexec_PROGRAMS = mate-netspeed-applet

BUILT_SOURCES = netspeed-resources.c netspeed-resources.h
nodist_mate_netspeed_applet_SOURCES = $(BUILT_SOURCES)
mate_netspeed_applet_SOURCES =	\
	backend.h		\
	backend.c		\
	netspeed.c		\
	$(NULL)

if HAVE_NL
mate_netspeed_applet_SOURCES +=	\
	nl80211.h		\
	ieee80211.h		\
	$(NULL)
endif

mate_netspeed_applet_LDADD = $(GIO_LIBS) $(GTOP_APPLETS_LIBS) \
       $(MATE_APPLETS4_LIBS) \
       $(INTLLIBS) -lm

if HAVE_IW
mate_netspeed_applet_LDADD += $(IW_LIBS)
endif
if HAVE_NL
mate_netspeed_applet_LDADD += $(NL_LIBS)
endif

netspeed-resources.c: $(srcdir)/../data/netspeed-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/netspeed-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name netspeed $<

netspeed-resources.h: $(srcdir)/../data/netspeed-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/netspeed-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name netspeed $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
