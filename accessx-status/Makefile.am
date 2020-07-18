NULL =

SUBDIRS = docs pixmaps

AM_CPPFLAGS = \
	${WARN_CFLAGS} \
	$(MATE_APPLETS4_CFLAGS) \
	$(GIO_CFLAGS) \
	-DACCESSX_PIXMAPS_DIR=\""$(datadir)/pixmaps/mate-accessx-status-applet"\" \
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

accessx-status-resources.c: accessx-status-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/accessx-status-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name accessx $<

accessx-status-resources.h: accessx-status-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/accessx-status-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name accessx $<

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.AccessxStatusApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.AccessxStatusAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.AccessxStatusAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
		-e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
		$< > $@

CLEANFILES = \
	$(applet_DATA) \
	$(applet_in_files) \
	$(service_DATA) \
	$(ACCESSX_STATUS_APPLET_CLEANFILES) \
	$(BUILT_SOURCES) \
	$(NULL)

EXTRA_DIST = \
	accessx-status-applet-menu.xml \
	accessx-status-resources.gresource.xml \
	$(applet_in_files).in \
	$(service_in_files) \
	$(NULL)


-include $(top_srcdir)/git.mk

