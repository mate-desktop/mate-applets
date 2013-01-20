serverdir = $(libdir)/matecomponent/servers
server_in_files = TimerApplet.server.in
server_DATA = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in) Makefile
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

schemasdir = $(MATECONF_SCHEMA_FILE_DIR)
schemas_in_files = timer-applet.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

pkgdata_DATA = \
	TimerApplet.xml \
	timer-applet.glade

EXTRA_DIST = \
	TimerApplet.server.in.in \
	$(pkgdata_DATA) \
	$(schemas_in_files)
	
CLEANFILES = \
	$(server_DATA) \
	$(server_in_files) \
	$(schemas_DATA)

@INTLTOOL_SERVER_RULE@
@INTLTOOL_SCHEMAS_RULE@
