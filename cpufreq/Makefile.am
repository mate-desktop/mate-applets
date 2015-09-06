SUBDIRS = help pixmaps src

@INTLTOOL_XML_NOMERGE_RULE@
cpufreq_gschema_in_files = org.mate.panel.applet.cpufreq.gschema.xml.in
gsettings_SCHEMAS = $(cpufreq_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.CPUFreqApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.CPUFreqAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.CPUFreqAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

builder_DATA = cpufreq-preferences.ui

uidir = $(datadir)/mate/ui
ui_DATA = cpufreq-applet-menu.xml

CLEANFILES = \
             $(applet_DATA)			\
             $(applet_DATA).in			\
             $(service_DATA)			\
	     $(gsettings_SCHEMAS)               \
	     *.gschema.valid                    \
	     *.bak                              \
	     *.gladep

EXTRA_DIST = \
             org.mate.applets.CPUFreqApplet.mate-panel-applet.in.in	\
             $(service_in_files)				\
	     $(cpufreq_gschema_in_files)			\
	     $(builder_DATA)                 			\
	     $(ui_DATA)
	     
					


-include $(top_srcdir)/git.mk
