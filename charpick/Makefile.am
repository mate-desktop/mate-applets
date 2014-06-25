SUBDIRS = help

AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	-DCHARPICK_MENU_UI_DIR=\""$(uidir)"\"	\
	$(MATE_APPLETS4_CFLAGS)		\
	$(MATEDESKTOP_CFLAGS)		\
	$(GUCHARMAP_CFLAGS)

libexec_PROGRAMS = charpick-applet

charpick_applet_SOURCES = charpick.c \
                          charpick.h \
                          properties.c

charpick_applet_LDADD = \
		       $(MATE_APPLETS4_LIBS)	\
		       $(MATEDESKTOP_LIBS)	\
		       $(GUCHARMAP_LIBS)

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.CharpickerApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.CharpickerAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.CharpickerAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

@INTLTOOL_XML_NOMERGE_RULE@
gsettings_SCHEMAS = org.mate.panel.applet.charpick.gschema.xml
@GSETTINGS_RULES@

%.gschema.xml.in: %.gschema.xml.in.in Makefile
	$(AM_V_GEN) $(SED) -e 's^\@GETTEXT_PACKAGE\@^$(GETTEXT_PACKAGE)^g' < $< > $@

uidir = $(datadir)/mate/ui
ui_DATA = charpick-applet-menu.xml

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(gsettings_SCHEMAS_in)  $(gsettings_SCHEMAS) *.gschema.valid

EXTRA_DIST =							\
	org.mate.applets.CharpickerApplet.mate-panel-applet.in.in	\
	$(service_in_files)					\
	$(ui_DATA)						\
	$(gsettings_SCHEMAS).in.in

-include $(top_srcdir)/git.mk
