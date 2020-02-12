SUBDIRS = pixmaps docs

ui_files = stickynotes-applet-menu.xml
builder_files =					\
	sticky-notes-delete.ui			\
	sticky-notes-delete-all.ui		\
	sticky-notes-note.ui			\
	sticky-notes-preferences.ui		\
	sticky-notes-properties.ui		\
	$(NULL)

applet_in_files = org.mate.applets.StickyNotesApplet.mate-panel-applet.desktop.in
service_in_files = org.mate.panel.applet.StickyNotesAppletFactory.service.in
schemas_in_files = stickynotes.schemas.in

if BUILD_STICKYNOTES_APPLET
AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	$(STICKYNOTES_CFLAGS)			\
	$(MATE_APPLETS4_CFLAGS)			\
	$(MATEDESKTOP_CFLAGS)			\
	$(LIBWNCK_CFLAGS)			\
	$(XML2_CFLAGS)				\
	-DSTICKYNOTES_MENU_UI_DIR=\""$(uidir)"\" \
	-DGRESOURCE=\""/org/mate/mate-applets/sticky-notes/\"" \
	$(NULL)

libexec_PROGRAMS = stickynotes-applet

stickynotes_applet_SOURCES =			\
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
	sticky-notes-resources.c		\
	sticky-notes-resources.h		\
	$(NULL)

stickynotes_applet_LDADD =			\
	$(STICKYNOTES_LIBS)			\
	$(MATE_APPLETS4_LIBS)			\
	$(MATEDESKTOP_LIBS)			\
	$(LIBWNCK_LIBS)				\
	$(XML2_LIBS)				\
	-lX11					\
	$(NULL)

uidir = $(datadir)/mate/ui
ui_DATA = $(ui_files)

stickynotes_gschema_in_files = org.mate.stickynotes.gschema.xml.in
gsettings_SCHEMAS = $(stickynotes_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

sticky-notes-resources.c: sticky-notes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/sticky-notes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name sticky_notes $<

sticky-notes-resources.h: sticky-notes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/sticky-notes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name sticky_notes $<

appletdir   = $(datadir)/mate-panel/applets
applet_DATA = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir   = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)

org.mate.panel.applet.StickyNotesAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES =					\
	$(applet_DATA)				\
	$(applet_in_files)			\
	$(service_DATA)				\
	$(gsettings_SCHEMAS)			\
	sticky-notes-resources.c		\
	sticky-notes-resources.h		\
	*.gschema.valid				\
	$(NULL)

endif

EXTRA_DIST =					\
	$(builder_files)			\
	$(ui_files)				\
	$(stickynotes_gschema_in_files)		\
	$(service_in_files)			\
	$(applet_in_files).in			\
	sticky-notes-resources.gresource.xml	\
	$(NULL)


-include $(top_srcdir)/git.mk
