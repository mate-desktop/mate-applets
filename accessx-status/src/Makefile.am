NULL =

AM_CPPFLAGS = \
	${WARN_CFLAGS} \
	$(MATE_APPLETS4_CFLAGS) \
	$(GIO_CFLAGS) \
	-DACCESSX_RESOURCE_PATH=\""/org/mate/mate-applets/accessx-status/"\" \
	$(NULL)

libexec_PROGRAMS = accessx-status-applet

BUILT_SOURCES = accessx-status-resources.c accessx-status-resources.h
nodist_accessx_status_applet_SOURCES = $(BUILT_SOURCES)
accessx_status_applet_SOURCES = \
	applet.c \
	applet.h

accessx_status_applet_LDADD = \
	$(MATE_APPLETS4_LIBS) \
	$(GIO_LIBS) \
	$(X_LIBS)

accessx-status-resources.c: $(srcdir)/../data/accessx-status-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/accessx-status-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name accessx $<

accessx-status-resources.h: $(srcdir)/../data/accessx-status-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/accessx-status-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name accessx $<

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

-include $(top_srcdir)/git.mk
