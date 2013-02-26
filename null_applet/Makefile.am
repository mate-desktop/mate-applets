AM_CPPFLAGS = 			\
	-I.			\
	-I$(srcdir)		\
	$(MATE_APPLETS4_CFLAGS)

libexec_PROGRAMS = null_applet

null_applet_SOURCES = null_applet.c

null_applet_LDADD = $(MATE_APPLETS4_LIBS)

serverdir = $(libdir)/matecomponent/servers
server_in_files =				\
	MATE_NullApplet_Factory.server.in	\
	MATE_CDPlayerApplet.server.in		\
	MATE_MailcheckApplet_Factory.server.in	\
	MATE_Panel_WirelessApplet.server.in	\
	MATE_KeyboardApplet.server.in
server_DATA = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

MATE_NullApplet_Factory.server.in: MATE_NullApplet_Factory.server.in.in
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

CLEANFILES = MATE_NullApplet_Factory.server.in $(server_DATA)

EXTRA_DIST =					\
	MATE_NullApplet_Factory.server.in.in	\
	MATE_CDPlayerApplet.server.in		\
	MATE_MailcheckApplet_Factory.server.in	\
	MATE_Panel_WirelessApplet.server.in	\
	MATE_KeyboardApplet.server.in

-include $(top_srcdir)/git.mk
