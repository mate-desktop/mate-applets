NULL =

mate_timer_applet_libdir= $(pkglibdir)
mate_timer_applet_lib_LTLIBRARIES=libmate-timer-applet.la

AM_CPPFLAGS =				\
	$(MATE_APPLETS4_CFLAGS)		\
	$(LIBNOTIFY_CFLAGS)		\
	-I$(srcdir)			\
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(NULL)

libmate_timer_applet_la_SOURCES = 		\
	timerapplet.c		\
    timerapplet-resources.c \
    timerapplet-resources.h  \
	$(NULL)

libmate_timer_applet_la_LIBADD =		\
	$(MATE_APPLETS4_LIBS)	\
	$(LIBNOTIFY_LIBS)	\
	$(NULL)

libmate_timer_applet_la_CFLAGS = $(WARN_CFLAGS)

timerapplet-resources.c: $(srcdir)/../data/timerapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/timerapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name timerapplet $<

timerapplet-resources.h: $(srcdir)/../data/timerapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/timerapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name timerapplet $<

CLEANFILES =			\
	$(NULL)

-include $(top_srcdir)/git.mk
