SUBDIRS = help

AM_CPPFLAGS =					\
	-I.					\
	-I$(srcdir)				\
	-DCHARPICK_MENU_UI_DIR=\""$(uidir)"\"	\
	$(MATE_APPLETS4_CFLAGS)		\
	$(MATEDESKTOP_CFLAGS)		\
	$(GUCHARMAP_CFLAGS)

libexec_PROGRAMS = mate-charpick-applet

mate_charpick_applet_SOURCES = charpick.c \
                          charpick.h \
                          properties.c

mate_charpick_applet_LDADD = \
		       $(MATE_APPLETS4_LIBS)	\
		       $(MATEDESKTOP_LIBS)	\
		       $(GUCHARMAP_LIBS)

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.CharpickerApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.CharpickerAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.CharpickerAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

charpick_gschema_in_files = org.mate.panel.applet.charpick.gschema.xml.in
gsettings_SCHEMAS = $(charpick_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

uidir = $(datadir)/mate/ui
ui_DATA = charpick-applet-menu.xml

CLEANFILES = $(applet_DATA)  $(applet_in_files) $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

EXTRA_DIST =							\
	$(applet_in_files).in	\
	$(service_in_files)					\
	$(ui_DATA)						\
	$(charpick_gschema_in_files)

-include $(top_srcdir)/git.mk
