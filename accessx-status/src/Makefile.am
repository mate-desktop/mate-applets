NULL =

AM_CPPFLAGS = \
	${WARN_CFLAGS} \
	$(MATE_APPLETS4_CFLAGS) \
	$(GIO_CFLAGS) \
	-DACCESSX_RESOURCE_PATH=\""/org/mate/mate-applets/accessx-status/"\" \
	$(NULL)

BUILT_SOURCES = accessx-status-resources.c accessx-status-resources.h
APPLET_SOURCES = \
	applet.c \
	applet.h

APPLET_LIBS = \
	$(MATE_APPLETS4_LIBS) \
	$(GIO_LIBS) \
	$(X_LIBS)

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmate-accessx-status-applet.la
nodist_libmate_accessx_status_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_accessx_status_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_accessx_status_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_accessx_status_applet_la_LDFLAGS = -module -avoid-version
libmate_accessx_status_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = accessx-status-applet
nodist_accessx_status_applet_SOURCES = $(BUILT_SOURCES)
accessx_status_applet_SOURCES = $(APPLET_SOURCES)
accessx_status_applet_CFLAGS = $(AM_CFLAGS)
accessx_status_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

accessx-status-resources.c: $(srcdir)/../data/accessx-status-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/accessx-status-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name accessx $<

accessx-status-resources.h: $(srcdir)/../data/accessx-status-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/accessx-status-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name accessx $<

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

-include $(top_srcdir)/git.mk
