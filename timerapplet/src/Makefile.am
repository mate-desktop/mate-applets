NULL =

AM_CPPFLAGS =				\
	$(MATE_APPLETS4_CFLAGS)		\
	$(LIBNOTIFY_CFLAGS)		\
	-I$(srcdir)			\
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(NULL)

libexec_PROGRAMS = timer-applet

BUILT_SOURCES = timerapplet-resources.c timerapplet-resources.h
nodist_timer_applet_SOURCES = $(BUILT_SOURCES)
timer_applet_SOURCES = 		\
	timerapplet.c		\
	$(NULL)

timer_applet_LDADD =		\
	$(MATE_APPLETS4_LIBS)	\
	$(LIBNOTIFY_LIBS)	\
	$(NULL)

timer_applet_CFLAGS = $(WARN_CFLAGS)

timerapplet-resources.c: $(srcdir)/../data/timerapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/timerapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name timerapplet $<

timerapplet-resources.h: $(srcdir)/../data/timerapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/timerapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name timerapplet $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
