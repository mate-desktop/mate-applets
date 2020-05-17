AM_CPPFLAGS = \
	$(MATE_APPLETS4_CFLAGS) \
	$(LIBNOTIFY_CFLAGS) \
	-I$(srcdir) \
	$(DISABLE_DEPRECATED_CFLAGS)

APPLET_LOCATION = $(libexecdir)/timer-applet

libexec_PROGRAMS = timer-applet
timer_applet_SOURCES = timerapplet.c
timer_applet_LDADD = $(MATE_APPLETS4_LIBS) $(LIBNOTIFY_LIBS)
timer_applet_CFLAGS = $(WARN_CFLAGS)

appletsdir       = $(datadir)/mate-panel/applets
applets_in_files = org.mate.applets.TimerApplet.mate-panel-applet.desktop.in
applets_DATA     = $(applets_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applets_in_files): $(applets_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@
$(applets_DATA): $(applets_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.TimerAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.TimerAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
		-e "s|\@LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@

timer_gschema_in_files = org.mate.panel.applet.timer.gschema.xml.in
gsettings_SCHEMAS = $(timer_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(applets_in_files).in \
	$(service_in_files) \
	$(timer_gschema_in_files)

CLEANFILES = $(applets_DATA) $(applets_in_files) $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

-include $(top_srcdir)/git.mk
