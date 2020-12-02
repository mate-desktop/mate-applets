AM_CPPFLAGS = \
	-I$(srcdir) \
	-DMULTILOAD_MENU_UI_DIR=\""$(datadir)/mate/ui/"\" \
	$(MATE_APPLETS4_CFLAGS) \
	$(GTOP_APPLETS_CFLAGS) \
	$(GIO_CFLAGS) \
	${WARN_CFLAGS}

libexec_PROGRAMS = mate-multiload-applet

mate_multiload_applet_SOURCES = \
	global.h \
	linux-proc.h \
	load-graph.h \
	linux-proc.c \
	load-graph.c \
	main.c \
	properties.c \
	netspeed.c netspeed.h \
	autoscaler.c \
	autoscaler.h

mate_multiload_applet_LDADD = \
	$(MATE_APPLETS4_LIBS) \
	$(GTOP_APPLETS_LIBS) \
	$(GIO_LIBS) \
	-lm

-include $(top_srcdir)/git.mk
