APPLET_LOCATION = $(libdir)/mate-applets/libcommand-applet.so

appletsdir       = $(datadir)/mate-panel/applets
applets_in_files = org.mate.applets.CommandApplet.mate-panel-applet.desktop.in
applets_DATA     = $(applets_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applets_in_files): $(applets_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@

$(applets_DATA): $(applets_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@


command_gschema_in_files = org.mate.panel.applet.command.gschema.xml.in
gsettings_SCHEMAS = $(command_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST = \
	$(applets_in_files).in \
	$(command_gschema_in_files) \
	command-preferences.ui \
	command-resources.gresource.xml

CLEANFILES = \
	$(applets_DATA) \
	$(applets_in_files) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid

-include $(top_srcdir)/git.mk
