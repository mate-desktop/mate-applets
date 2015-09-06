SUBDIRS = docs

AM_CPPFLAGS =			\
	-I$(srcdir)		\
	-DMULTILOAD_MENU_UI_DIR=\""$(uidir)"\" \
	$(MATE_APPLETS4_CFLAGS)	\
	$(GTOP_APPLETS_CFLAGS)	\
	$(MATEDESKTOP_CFLAGS)	\
	$(GIO_CFLAGS)

libexec_PROGRAMS = mate-multiload-applet

mate_multiload_applet_SOURCES = \
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

mate_multiload_applet_LDADD = \
				$(MATE_APPLETS4_LIBS) 	\
				$(GTOP_APPLETS_LIBS)	\
				$(MATEDESKTOP_LIBS)		\
				$(GIO_LIBS)				\
				-lm

@INTLTOOL_XML_NOMERGE_RULE@
multiload_gschema_in_files = org.mate.panel.applet.multiload.gschema.xml.in
gsettings_SCHEMAS = $(multiload_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST =							\
	org.mate.applets.MultiLoadApplet.mate-panel-applet.in.in	\
	$(service_in_files)					\
	$(multiload_gschema_in_files)				\
	$(ui_DATA)

uidir		= $(datadir)/mate/ui
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

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

-include $(top_srcdir)/git.mk
