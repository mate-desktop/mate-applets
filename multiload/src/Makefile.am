NULL =

mate_multiload_applet_libdir= $(pkglibdir)
mate_multiload_applet_lib_LTLIBRARIES=libmate-multiload-applet.la

AM_CPPFLAGS = \
	-I$(srcdir) \
	-DMULTILOAD_RESOURCE_PATH=\""/org/mate/mate-applets/multiload/"\" \
	$(MATE_APPLETS4_CFLAGS) \
	$(GTOP_APPLETS_CFLAGS) \
	$(GIO_CFLAGS) \
	${WARN_CFLAGS}

libmate_multiload_applet_la_SOURCES = \
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
	multiload-resources.c \
	multiload-resources.h \
	$(NULL)

libmate_multiload_applet_la_LIBADD = \
	$(MATE_APPLETS4_LIBS) \
	$(GTOP_APPLETS_LIBS) \
	$(GIO_LIBS) \
	-lm

multiload-resources.c: $(srcdir)/../data/multiload-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/multiload-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name multiload $<

multiload-resources.h: $(srcdir)/../data/multiload-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/multiload-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name multiload $<

CLEANFILES = \
	$(NULL)

-include $(top_srcdir)/git.mk
