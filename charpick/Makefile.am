NULL =

APPLET_LOCATION = $(libdir)/mate-applets/libmate-charpick-applet.so

SUBDIRS = help

mate_charpick_applet_libdir= $(pkglibdir)
mate_charpick_applet_lib_LTLIBRARIES=libmate-charpick-applet.la

AM_CPPFLAGS =			\
	-I.			\
	-I$(srcdir)		\
	-DCHARPICK_RESOURCE_PATH=\""/org/mate/mate-applets/charpick/"\"	\
	${WARN_CFLAGS}		\
	$(MATE_APPLETS4_CFLAGS)	\
	$(GUCHARMAP_CFLAGS)	\
	$(NULL)

libmate_charpick_applet_la_SOURCES = 	\
	charpick.c		\
	charpick.h		\
	properties.c		\
	charpick-resources.c \
	charpick-resources.h \
	$(NULL)

libmate_charpick_applet_la_LIBADD =	\
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
		-e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
		-e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
		$< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

charpick_gschema_in_files = org.mate.panel.applet.charpick.gschema.xml.in
gsettings_SCHEMAS = $(charpick_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

CLEANFILES =			\
	$(applet_DATA)		\
	$(applet_in_files)	\
	$(gsettings_SCHEMAS)	\
	*.gschema.valid		\
	$(NULL)

EXTRA_DIST =					\
	$(applet_in_files).in			\
	$(charpick_gschema_in_files)		\
	charpick-applet-menu.xml		\
	charpick-resources.gresource.xml	\
	$(NULL)

-include $(top_srcdir)/git.mk
