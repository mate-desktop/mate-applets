NULL =

SUBDIRS = pixmaps docs

applet_in_files = org.mate.applets.StickyNotesApplet.mate-panel-applet.desktop.in
service_in_files = org.mate.panel.applet.StickyNotesAppletFactory.service.in
schemas_in_files = stickynotes.schemas.in

if BUILD_STICKYNOTES_APPLET
AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	$(STICKYNOTES_CFLAGS)			\
	$(MATE_APPLETS4_CFLAGS)			\
	$(LIBWNCK_CFLAGS)			\
	$(XML2_CFLAGS)				\
	-DGRESOURCE=\""/org/mate/mate-applets/sticky-notes/\"" \
	${WARN_CFLAGS}				\
	$(NULL)

BUILT_SOURCES =					\
	sticky-notes-resources.c		\
	sticky-notes-resources.h		\
	$(NULL)
APPLET_SOURCES =				\
	util.h					\
	util.c					\
	stickynotes.h				\
	stickynotes_callbacks.h			\
	stickynotes_applet.h			\
	stickynotes_applet_callbacks.h		\
	stickynotes.c				\
	stickynotes_callbacks.c			\
	stickynotes_applet.c			\
	stickynotes_applet_callbacks.c		\
	$(NULL)

APPLET_LIBS =					\
	$(STICKYNOTES_LIBS)			\
	$(MATE_APPLETS4_LIBS)			\
	$(LIBWNCK_LIBS)				\
	$(XML2_LIBS)				\
	-lX11					\
	$(NULL)

if ENABLE_IN_PROCESS
APPLET_LOCATION = $(pkglibdir)/libmate-stickynotes-applet.so
pkglib_LTLIBRARIES = libmate-stickynotes-applet.la
nodist_libmate_stickynotes_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_stickynotes_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_stickynotes_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_stickynotes_applet_la_LDFLAGS = -module -avoid-version
libmate_stickynotes_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
APPLET_LOCATION = $(libexecdir)/stickynotes-applet
libexec_PROGRAMS = stickynotes-applet
nodist_stickynotes_applet_SOURCES = $(BUILT_SOURCES)
stickynotes_applet_SOURCES = $(APPLET_SOURCES)
stickynotes_applet_CFLAGS = $(AM_CFLAGS)
stickynotes_applet_LDADD = $(APPLET_LIBS)

servicedir = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@
endif !ENABLE_IN_PROCESS

stickynotes_gschema_in_files = org.mate.stickynotes.gschema.xml.in
gsettings_SCHEMAS = $(stickynotes_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

sticky-notes-resources.c: sticky-notes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/sticky-notes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name sticky_notes $<

sticky-notes-resources.h: sticky-notes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/sticky-notes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name sticky_notes $<

appletdir   = $(datadir)/mate-panel/applets
applet_DATA = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files:=.in) Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@APPLET_IN_PROCESS\@|$(APPLET_IN_PROCESS)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

CLEANFILES =					\
	$(applet_DATA)				\
	$(applet_in_files)			\
	$(service_DATA)				\
	$(gsettings_SCHEMAS)			\
	sticky-notes-resources.c		\
	sticky-notes-resources.h		\
	*.gschema.valid				\
	$(BUILT_SOURCES)			\
	$(NULL)

endif

EXTRA_DIST =					\
	$(stickynotes_gschema_in_files)		\
	$(service_in_files)			\
	$(applet_in_files:=.in)			\
	sticky-notes-delete.ui                  \
	sticky-notes-delete-all.ui              \
	sticky-notes-note.ui                    \
	sticky-notes-preferences.ui             \
	sticky-notes-properties.ui              \
	sticky-notes-resources.gresource.xml    \
	stickynotes-applet-menu.xml		\
	$(NULL)

-include $(top_srcdir)/git.mk
