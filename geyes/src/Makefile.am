NULL =

AM_CPPFLAGS =						\
	-I.						\
	-I$(srcdir) 					\
	$(MATE_APPLETS4_CFLAGS)				\
	-I$(includedir) 				\
	-DGEYES_THEMES_DIR=\""$(pkgdatadir)/geyes/"\"	\
	-DGEYES_RESOURCE_PATH=\""/org/mate/mate-applets/eyes/"\" \
	${WARN_CFLAGS}

BUILT_SOURCES = geyes-resources.c geyes-resources.h
APPLET_SOURCES =		\
	geyes.c			\
	geyes.h			\
	themes.c		\
	$(NULL)
APPLET_LIBS =			\
	$(MATE_APPLETS4_LIBS)	\
	-lm

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmate-geyes-applet.la
nodist_libmate_geyes_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_geyes_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_geyes_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_geyes_applet_la_LDFLAGS = -module -avoid-version
libmate_geyes_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = mate-geyes-applet
nodist_mate_geyes_applet_SOURCES = $(BUILT_SOURCES)
mate_geyes_applet_SOURCES = $(APPLET_SOURCES)
mate_geyes_applet_CFLAGS = $(AM_CFLAGS)
mate_geyes_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

geyes-resources.c: $(srcdir)/../data/geyes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/geyes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name eyes $<

geyes-resources.h: $(srcdir)/../data/geyes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/geyes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name eyes $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
