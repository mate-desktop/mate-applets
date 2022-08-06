NULL =

AM_CPPFLAGS =						\
	-I.						\
	-I$(srcdir) 					\
	$(MATE_APPLETS4_CFLAGS)				\
	-I$(includedir) 				\
	-DGEYES_THEMES_DIR=\""$(pkgdatadir)/geyes/"\"	\
	${WARN_CFLAGS}

libexec_PROGRAMS = mate-geyes-applet

BUILT_SOURCES = geyes-resources.c geyes-resources.h
nodist_mate_geyes_applet_SOURCES = $(BUILT_SOURCES)
mate_geyes_applet_SOURCES =	\
	geyes.c			\
	geyes.h			\
	themes.c		\
	$(NULL)

mate_geyes_applet_LDADD =	\
	$(MATE_APPLETS4_LIBS)	\
	-lm

geyes-resources.c: $(srcdir)/../data/geyes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/geyes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name eyes $<

geyes-resources.h: $(srcdir)/../data/geyes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/geyes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name eyes $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
