NULL =

builder_files =			\
	netspeed-details.ui	\
	netspeed-preferences.ui	\
	$(NULL)

AM_CPPFLAGS =			\
	-I$(top_srcdir)		\
	-I$(includedir)		\
	$(GIO_CFLAGS)		\
	$(GTOP_APPLETS_CFLAGS)	\
	$(MATE_APPLETS4_CFLAGS)	\
	-DNETSPEED_MENU_UI_DIR=\""$(datadir)/mate/ui"\" \
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

netspeed-resources.c: netspeed-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/netspeed-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name netspeed $<

netspeed-resources.h: netspeed-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/netspeed-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name netspeed $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

EXTRA_DIST =					\
	$(builder_files)			\
	netspeed-resources.gresource.xml	\
	$(NULL)

-include $(top_srcdir)/git.mk
