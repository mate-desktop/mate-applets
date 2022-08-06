AM_CPPFLAGS =					\
	-I$(srcdir)				\
	-I$(top_srcdir)				\
	$(MATE_APPLETS4_CFLAGS)			\
	$(LIBNOTIFY_CFLAGS)			\
	$(LIBMATEWEATHER_CFLAGS)		\
	${WARN_CFLAGS}

libexec_PROGRAMS = mateweather-applet

BUILT_SOURCES = mateweather-resources.c mateweather-resources.h
nodist_mateweather_applet_SOURCES = $(BUILT_SOURCES)
mateweather_applet_SOURCES = \
	mateweather.h \
	main.c \
	mateweather-about.c mateweather-about.h \
	mateweather-pref.c mateweather-pref.h \
	mateweather-dialog.c mateweather-dialog.h \
	mateweather-applet.c mateweather-applet.h

mateweather_applet_LDADD = 	\
	$(LIBNOTIFY_LIBS) 	\
	$(MATE_APPLETS4_LIBS)	\
	$(MATE_LIBS2_LIBS)	\
	$(LIBMATEWEATHER_LIBS)

mateweather-resources.c: ../data/mateweather-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/mateweather-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name mateweather $<

mateweather-resources.h: ../data/mateweather-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/mateweather-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name mateweather $<

CLEANFILES =			\
	$(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
