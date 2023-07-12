NULL =

AM_CPPFLAGS =				\
	$(MATE_APPLETS4_CFLAGS)		\
	$(LIBNOTIFY_CFLAGS)		\
	-I$(srcdir)			\
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(NULL)

AM_CFLAGS = $(WARN_CFLAGS)

BUILT_SOURCES =			\
	timerapplet-resources.c	\
	timerapplet-resources.h	\
	$(NULL)
APPLET_SOURCES = 		\
	timerapplet.c	\
	$(NULL)

APPLET_LIBS =			\
	$(MATE_APPLETS4_LIBS)	\
	$(LIBNOTIFY_LIBS)	\
	$(NULL)

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmate-timer-applet.la
nodist_libmate_timer_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_timer_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_timer_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_timer_applet_la_LDFLAGS = -module -avoid-version
libmate_timer_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = timer-applet
nodist_timer_applet_SOURCES = $(BUILT_SOURCES)
timer_applet_SOURCES = $(APPLET_SOURCES)
timer_applet_CFLAGS = $(AM_CFLAGS)
timer_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

timerapplet-resources.c: $(srcdir)/../data/timerapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/timerapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name timerapplet $<

timerapplet-resources.h: $(srcdir)/../data/timerapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/timerapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name timerapplet $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
