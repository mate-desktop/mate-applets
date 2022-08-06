NULL =

SUBDIRS = help

AM_CPPFLAGS =			\
	-I.			\
	-I$(srcdir)		\
	${WARN_CFLAGS}		\
	$(MATE_APPLETS4_CFLAGS)	\
	$(GUCHARMAP_CFLAGS)	\
	$(NULL)

libexec_PROGRAMS = mate-charpick-applet

BUILT_SOURCES = charpick-resources.c charpick-resources.h
nodist_mate_charpick_applet_SOURCES = $(BUILT_SOURCES)
mate_charpick_applet_SOURCES = 	\
	charpick.c		\
	charpick.h		\
	properties.c		\
	$(NULL)

mate_charpick_applet_LDADD =	\
	$(MATE_APPLETS4_LIBS)	\
	$(GUCHARMAP_LIBS)	\
	$(NULL)

charpick-resources.c: charpick-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/charpick-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name charpick $<

charpick-resources.h: charpick-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/charpick-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name charpick $<

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

CLEANFILES =			\
	$(applet_DATA)		\
	$(applet_in_files)	\
	$(service_DATA)		\
	$(gsettings_SCHEMAS)	\
	$(BUILT_SOURCES)	\
	*.gschema.valid		\
	$(NULL)

EXTRA_DIST =					\
	$(applet_in_files).in			\
	$(service_in_files)			\
	$(charpick_gschema_in_files)		\
	menu.xml				\
	charpick-resources.gresource.xml.in	\
	$(NULL)

-include $(top_srcdir)/git.mk
