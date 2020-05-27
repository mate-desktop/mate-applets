AM_CPPFLAGS = \
	$(MATE_APPLETS4_CFLAGS) \
	-I$(srcdir) \
	$(DISABLE_DEPRECATED_CFLAGS)

APPLET_LOCATION = $(libexecdir)/command-applet

libexec_PROGRAMS = command-applet

BUILT_SOURCES = command-resources.c command-resources.h
nodist_command_applet_SOURCES = $(BUILT_SOURCES)
command_applet_SOURCES = command.c ma-command.c ma-command.h
command_applet_LDADD = $(MATE_APPLETS4_LIBS)
command_applet_CFLAGS = $(WARN_CFLAGS)

command-resources.c: command-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/command-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name command $<

command-resources.h: command-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/command-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name command $<

appletsdir       = $(datadir)/mate-panel/applets
applets_in_files = org.mate.applets.CommandApplet.mate-panel-applet.desktop.in
applets_DATA     = $(applets_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applets_in_files): $(applets_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@

$(applets_DATA): $(applets_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.CommandAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.CommandAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
		-e "s|\@LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@

command_gschema_in_files = org.mate.panel.applet.command.gschema.xml.in
gsettings_SCHEMAS = $(command_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(applets_in_files).in \
	$(service_in_files) \
	$(command_gschema_in_files) \
	command-preferences.ui \
	command-resources.gresource.xml

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(applets_DATA) \
	$(applets_in_files) \
	$(service_DATA) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid

-include $(top_srcdir)/git.mk
