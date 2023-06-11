NULL =

cpufreq_gschema_in_files = org.mate.panel.applet.cpufreq.gschema.xml.in
gsettings_SCHEMAS = $(cpufreq_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

APPLET_LOCATION = $(libdir)/mate-applets/libmatecpufreqapplet.so

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.CPUFreqApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@


CLEANFILES =				\
	$(applet_DATA)			\
	$(applet_in_files)		\
	$(gsettings_SCHEMAS)		\
	*.gschema.valid			\
	$(NULL)

EXTRA_DIST =				\
	$(applet_in_files).in		\
	$(cpufreq_gschema_in_files)	\
	cpufreq-applet-menu.xml		\
	cpufreq-preferences.ui		\
	cpufreq-resources.gresource.xml	\
	$(NULL)

-include $(top_srcdir)/git.mk
