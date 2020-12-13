NULL =

AM_CPPFLAGS = \
	-I$(srcdir) \
	-DMULTILOAD_RESOURCE_PATH=\""/org/mate/mate-applets/multiload/"\" \
	$(MATE_APPLETS4_CFLAGS) \
	$(GTOP_APPLETS_CFLAGS) \
	$(GIO_CFLAGS) \
	${WARN_CFLAGS}

libexec_PROGRAMS = mate-multiload-applet

BUILT_SOURCES = multiload-resources.c multiload-resources.h
nodist_mate_multiload_applet_SOURCES = $(BUILT_SOURCES)
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

multiload-resources.c: $(srcdir)/../data/multiload-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/multiload-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name multiload $<

multiload-resources.h: $(srcdir)/../data/multiload-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/multiload-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name multiload $<

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

-include $(top_srcdir)/git.mk
