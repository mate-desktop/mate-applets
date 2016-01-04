## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps docs

ui_files = stickynotes-applet-menu.xml
builder_files = stickynotes.ui
applet_in_files = org.mate.applets.StickyNotesApplet.mate-panel-applet.in
service_in_files = org.mate.panel.applet.StickyNotesAppletFactory.service.in
schemas_in_files = stickynotes.schemas.in

if BUILD_STICKYNOTES_APPLET
AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	$(STICKYNOTES_CFLAGS)		\
	$(MATE_APPLETS4_CFLAGS)		\
	$(MATEDESKTOP_CFLAGS)		\
	$(LIBWNCK_CFLAGS)			\
	$(XML2_CFLAGS)				\
	-DSTICKYNOTES_MENU_UI_DIR=\""$(uidir)"\"

libexec_PROGRAMS = stickynotes-applet

stickynotes_applet_SOURCES = \
	util.h \
	util.c \
	stickynotes.h \
	stickynotes_callbacks.h \
	stickynotes_applet.h \
	stickynotes_applet_callbacks.h \
	stickynotes.c \
	stickynotes_callbacks.c \
	stickynotes_applet.c \
	stickynotes_applet_callbacks.c

stickynotes_applet_LDADD =	\
	$(STICKYNOTES_LIBS)	\
	$(MATE_APPLETS4_LIBS)	\
	$(MATEDESKTOP_LIBS)	\
	$(LIBWNCK_LIBS)		\
	$(XML2_LIBS) \
	-lX11

builder_DATA = $(builder_files)

uidir = $(datadir)/mate/ui
ui_DATA = $(ui_files)

@INTLTOOL_XML_NOMERGE_RULE@
stickynotes_gschema_in_files = org.mate.stickynotes.gschema.xml.in
gsettings_SCHEMAS = $(stickynotes_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

appletdir   = $(datadir)/mate-panel/applets
applet_DATA = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir   = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)

org.mate.panel.applet.StickyNotesAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

endif

EXTRA_DIST = \
	     $(builder_files) \
	     $(ui_files) \
	     $(stickynotes_gschema_in_files) \
             $(service_in_files) \
	     org.mate.applets.StickyNotesApplet.mate-panel-applet.in.in


-include $(top_srcdir)/git.mk
