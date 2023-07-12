NULL =

SUBDIRS = help

applet_in_files = org.mate.applets.CharpickerApplet.mate-panel-applet.desktop.in
service_in_files = org.mate.panel.applet.CharpickerAppletFactory.service.in
gschema_in_files = org.mate.panel.applet.charpick.gschema.xml.in

AM_CPPFLAGS =			\
	-I.			\
	-I$(srcdir)		\
	-DCHARPICK_RESOURCE_PATH=\""/org/mate/mate-applets/charpick/"\"	\
	${WARN_CFLAGS}		\
	$(MATE_APPLETS4_CFLAGS)	\
	$(GUCHARMAP_CFLAGS)	\
	$(NULL)

BUILT_SOURCES =			\
	charpick-resources.c	\
	charpick-resources.h	\
	$(NULL)
APPLET_SOURCES =		\
	charpick.c		\
	charpick.h		\
	properties.c		\
	$(NULL)

APPLET_LIBS =			\
	$(MATE_APPLETS4_LIBS)	\
	$(GUCHARMAP_LIBS)	\
	$(NULL)

if ENABLE_IN_PROCESS
APPLET_LOCATION = $(pkglibdir)/libmate-charpick-applet.so

pkglib_LTLIBRARIES = libmate-charpick-applet.la
nodist_libmate_charpick_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_charpick_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_charpick_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_charpick_applet_la_LDFLAGS = -module -avoid-version
libmate_charpick_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
APPLET_LOCATION = $(libexecdir)/mate-charpick-applet

libexec_PROGRAMS = mate-charpick-applet
nodist_mate_charpick_applet_SOURCES = $(BUILT_SOURCES)
mate_charpick_applet_SOURCES = $(APPLET_SOURCES)
mate_charpick_applet_CFLAGS = $(AM_CFLAGS)
mate_charpick_applet_LDADD = $(APPLET_LIBS)

servicedir = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		$< > $@
endif !ENABLE_IN_PROCESS

charpick-resources.c: charpick-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/charpick-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name charpick $<

charpick-resources.h: charpick-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/charpick-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name charpick $<

appletdir       = $(datadir)/mate-panel/applets
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@APPLET_IN_PROCESS\@|$(APPLET_IN_PROCESS)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

gsettings_SCHEMAS = $(gschema_in_files:.xml.in=.xml)
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
	$(gschema_in_files)			\
	charpick-applet-menu.xml		\
	charpick-resources.gresource.xml	\
	$(NULL)

-include $(top_srcdir)/git.mk
