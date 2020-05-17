AM_CPPFLAGS =			\
	-I$(top_srcdir)		\
	-I$(includedir)		\
	$(GIO_CFLAGS)		\
	$(GTOP_APPLETS_CFLAGS)	\
	$(MATE_APPLETS4_CFLAGS)	\
	-DNETSPEED_MENU_UI_DIR=\""$(datadir)/mate/ui"\" \
	${WARN_CFLAGS}

libexec_PROGRAMS = mate-netspeed-applet

mate_netspeed_applet_SOURCES = backend.h backend.c netspeed.c

mate_netspeed_applet_LDADD = $(GIO_LIBS) $(GTOP_APPLETS_LIBS) \
       $(MATE_APPLETS4_LIBS) \
       $(IWLIB) $(INTLLIBS) -lm


-include $(top_srcdir)/git.mk
