SUBDIRS = docs

INCLUDES =			\
	-I$(srcdir)		\
	-DMULTILOAD_MENU_UI_DIR=\""$(uidir)"\" \
	$(MATE_APPLETS4_CFLAGS)	\
	$(GTOP_APPLETS_CFLAGS)	\
	$(MATEDESKTOP_CFLAGS)	\
	$(GIO_CFLAGS)

libexec_PROGRAMS = multiload-applet-2

multiload_applet_2_SOURCES = \
			global.h	\
			linux-proc.h	\
			load-graph.h	\
			linux-proc.c	\
			load-graph.c	\
			main.c	\
			properties.c	\
			netspeed.c netspeed.h \
			autoscaler.c	\
			autoscaler.h

multiload_applet_2_LDADD = \
				$(MATE_APPLETS4_LIBS) 	\
				$(GTOP_APPLETS_LIBS)	\
				$(MATEDESKTOP_LIBS)		\
				$(GIO_LIBS)				\
				-lm

@INTLTOOL_XML_NOMERGE_RULE@
gsettings_SCHEMAS = org.mate.panel.applet.multiload.gschema.xml
@GSETTINGS_RULES@

%.gschema.xml.in: %.gschema.xml.in.in Makefile
	$(AM_V_GEN) $(SED) -e 's^\@GETTEXT_PACKAGE\@^$(GETTEXT_PACKAGE)^g' < $< > $@

convertdir = $(datadir)/MateConf/gsettings
convert_DATA = multiload-applet.convert

EXTRA_DIST =							\
	org.mate.applets.MultiLoadApplet.mate-panel-applet.in.in	\
	$(service_in_files)					\
	$(gsettings_SCHEMAS_in_in)			\
	$(convert_DATA)					\
	$(ui_DATA)

uidir		= $(datadir)/mate-2.0/ui
ui_DATA		= multiload-applet-menu.xml

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.MultiLoadApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.MultiLoadAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.MultiLoadAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(gsettings_SCHEMAS_in)  $(gsettings_SCHEMAS) *.gschema.valid

-include $(top_srcdir)/git.mk
