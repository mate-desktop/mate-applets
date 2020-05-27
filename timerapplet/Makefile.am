NULL =

AM_CPPFLAGS = \
	$(MATE_APPLETS4_CFLAGS) \
	$(LIBNOTIFY_CFLAGS) \
	-I$(srcdir) \
	$(DISABLE_DEPRECATED_CFLAGS)

APPLET_LOCATION = $(libexecdir)/timer-applet

libexec_PROGRAMS = timer-applet

BUILT_SOURCES = timerapplet-resources.c timerapplet-resources.h
nodist_timer_applet_SOURCES = $(BUILT_SOURCES)
timer_applet_SOURCES = 		\
	timerapplet.c		\
	$(NULL)

timer_applet_LDADD =		\
	$(MATE_APPLETS4_LIBS)	\
	$(LIBNOTIFY_LIBS)	\
	$(NULL)

timer_applet_CFLAGS = $(WARN_CFLAGS)

timerapplet-resources.c: timerapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/timerapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name timerapplet $<

timerapplet-resources.h: timerapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/timerapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name timerapplet $<

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

EXTRA_DIST =					\
	$(applets_in_files).in			\
	$(service_in_files)			\
	$(timer_gschema_in_files)		\
	timerapplet-preferences.ui		\
	timerapplet-resources.gresource.xml	\
	$(NULL)

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(applets_DATA)		\
	$(applets_in_files)	\
	$(service_DATA)		\
	$(gsettings_SCHEMAS)	\
	*.gschema.valid		\
	$(NULL)

-include $(top_srcdir)/git.mk
