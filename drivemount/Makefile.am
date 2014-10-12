SUBDIRS = help

AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	-DDRIVEMOUNT_MENU_UI_DIR=\""$(uidir)"\"	\
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


appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.DriveMountApplet.mate-panel-applet.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.mate-panel-applet: %.mate-panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.DriveMountAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.DriveMountAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

uidir = $(datadir)/mate/ui
ui_DATA = drivemount-applet-menu.xml

CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA)

EXTRA_DIST = \
 org.mate.applets.DriveMountApplet.mate-panel-applet.in.in	\
 $(service_in_files)					\
 $(ui_DATA)

-include $(top_srcdir)/git.mk
