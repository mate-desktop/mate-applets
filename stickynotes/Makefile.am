## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps docs data

ui_files = stickynotes-applet-menu.xml
builder_files = stickynotes.ui
applet_in_files = org.mate.applets.StickyNotesApplet.mate-panel-applet.in
service_in_files = org.mate.panel.applet.StickyNotesAppletFactory.service.in
schemas_in_files = stickynotes.schemas.in

if BUILD_STICKYNOTES_APPLET
AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	$(MATE_APPLETS4_CFLAGS)		\
	$(LIBMATEWNCK_CFLAGS)			\
	$(XML2_CFLAGS)				\
	-DSTICKYNOTES_MENU_UI_DIR=\""$(uidir)"\"\
	-DSTICKYNOTES_ICONDIR=\"$(datadir)/pixmaps/mate-stickynotes/\"

libexec_PROGRAMS = stickynotes_applet

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
	$(MATE_APPLETS4_LIBS)	\
	$(LIBMATEWNCK_LIBS)		\
	$(XML2_LIBS) \
	-lX11

builder_DATA = $(builder_files)

uidir = $(datadir)/mate-2.0/ui
ui_DATA = $(ui_files)

@INTLTOOL_XML_NOMERGE_RULE@
gsettings_SCHEMAS = org.mate.stickynotes.gschema.xml
@GSETTINGS_RULES@

%.gschema.xml.in: %.gschema.xml.in.in Makefile
	$(AM_V_GEN) $(SED) -e 's^\@GETTEXT_PACKAGE\@^$(GETTEXT_PACKAGE)^g' < $< > $@

convertdir = $(datadir)/MateConf/gsettings
convert_DATA = stickynotes-applet.convert

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

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(gsettings_SCHEMAS_in)  $(gsettings_SCHEMAS) *.gschema.valid

endif

EXTRA_DIST = \
	     $(builder_files) \
	     $(ui_files) \
	     $(gsettings_SCHEMAS_in_in) \
	     $(convert_DATA) \
             $(service_in_files) \
	     org.mate.applets.StickyNotesApplet.mate-panel-applet.in.in


-include $(top_srcdir)/git.mk
