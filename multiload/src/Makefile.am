NULL =

AM_CPPFLAGS = \
	-I$(srcdir) \
	-DMULTILOAD_RESOURCE_PATH=\""/org/mate/mate-applets/multiload/"\" \
	$(MATE_APPLETS4_CFLAGS) \
	$(GTOP_APPLETS_CFLAGS) \
	$(GIO_CFLAGS) \
	${WARN_CFLAGS}

BUILT_SOURCES = \
	multiload-resources.c \
	multiload-resources.h \
	$(NULL)
APPLET_SOURCES = \
	global.h \
	linux-proc.h \
	load-graph.h \
	linux-proc.c \
	load-graph.c \
	main.c \
	properties.c \
	netspeed.c netspeed.h \
	autoscaler.c \
	autoscaler.h \
	$(NULL)

APPLET_LIBS = \
	$(MATE_APPLETS4_LIBS) \
	$(GTOP_APPLETS_LIBS) \
	$(GIO_LIBS) \
	-lm

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmate-multiload-applet.la
nodist_libmate_multiload_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_multiload_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_multiload_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_multiload_applet_la_LDFLAGS = -module -avoid-version
libmate_multiload_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = mate-multiload-applet
nodist_mate_multiload_applet_SOURCES = $(BUILT_SOURCES)
mate_multiload_applet_SOURCES = $(APPLET_SOURCES)
mate_multiload_applet_CFLAGS = $(AM_CFLAGS)
mate_multiload_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

multiload-resources.c: $(srcdir)/../data/multiload-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/multiload-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name multiload $<

multiload-resources.h: $(srcdir)/../data/multiload-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/multiload-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name multiload $<

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

-include $(top_srcdir)/git.mk
