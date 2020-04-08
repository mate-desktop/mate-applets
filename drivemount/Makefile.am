SUBDIRS = help

AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	-DDRIVEMOUNT_MENU_UI_DIR=\""$(uidir)"\"	\
	${WARN_CFLAGS}				\
	$(MATE_APPLETS4_CFLAGS)			\
	$(MATEDESKTOP_CFLAGS)

libexec_PROGRAMS = mate-drivemount-applet

mate_drivemount_applet_SOURCES =	\
	drivemount.c		\
	drive-list.c		\
	drive-list.h		\
	drive-button.c		\
	drive-button.h

mate_drivemount_applet_LDADD =	\
	$(MATE_APPLETS4_LIBS)	\
	$(MATEDESKTOP_LIBS)

uidir           = $(datadir)/mate/ui
ui_DATA         = drivemount-applet-menu.xml

drivemount_gschema_in_files = org.mate.drivemount.gschema.xml.in
gsettings_SCHEMAS = $(drivemount_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

EXTRA_DIST =                                                    \
        $(applet_in_files).in                                   \
        $(service_in_files)                                     \
        $(drivemount_gschema_in_files)                          \
        $(ui_DATA)


appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.DriveMountApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.DriveMountAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.DriveMountAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_in_files) $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

-include $(top_srcdir)/git.mk
