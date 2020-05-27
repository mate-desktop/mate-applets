NULL =

SUBDIRS = themes docs

AM_CPPFLAGS =						\
	-I.						\
	-I$(srcdir) 					\
	$(MATE_APPLETS4_CFLAGS)				\
	-I$(includedir) 				\
	-DGEYES_THEMES_DIR=\""$(pkgdatadir)/geyes/"\"	\
	-DGEYES_MENU_UI_DIR=\""$(uidir)"\"		\
	${WARN_CFLAGS}

libexec_PROGRAMS = mate-geyes-applet

BUILT_SOURCES = geyes-resources.c geyes-resources.h
nodist_mate_geyes_applet_SOURCES = $(BUILT_SOURCES)
mate_geyes_applet_SOURCES =	\
	geyes.c			\
	geyes.h			\
	themes.c		\
	$(NULL)

mate_geyes_applet_LDADD =	\
	$(MATE_APPLETS4_LIBS)	\
	-lm

geyes-resources.c: geyes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/geyes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name eyes $<

geyes-resources.h: geyes-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/geyes-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name eyes $<

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.GeyesApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.GeyesAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.GeyesAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

uidir = $(datadir)/mate/ui
ui_DATA = geyes-applet-menu.xml

geyes_gschema_in_files = org.mate.panel.applet.geyes.gschema.xml.in
gsettings_SCHEMAS = $(geyes_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

themesdir = $(pkgdatadir)/geyes

icons =	\
	mate-eyes-applet.svg		\
	mate-eyes-applet.256.png	\
	mate-eyes-applet.64.png		\
	mate-eyes-applet.32.png		\
	mate-eyes-applet.24.png		\
	mate-eyes-applet.22.png		\
	mate-eyes-applet.16.png

CLEANFILES = \
	$(BUILT_SOURCES)		\
	$(applet_DATA)			\
	$(applet_in_files)		\
	$(service_DATA)			\
	$(gsettings_SCHEMAS)		\
	*.gschema.valid			\
	$(NULL)

EXTRA_DIST = \
	geyes-resources.gresource.xml	\
	themes.ui			\
	README.themes			\
	$(applet_in_files).in		\
	$(service_in_files)		\
	$(geyes_gschema_in_files)	\
	$(ui_DATA)			\
	$(icons)			\
	$(NULL)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

uninstall-local:
	for icon in $(icons); do \
		SIZE=`echo $$icon | cut -d. -f2`; \
		FILE=`echo $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		rm -f $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/$$FILE; \
	done
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

install-data-local:
	for icon in $(icons); do \
		SIZE=`echo $$icon | cut -d. -f2`; \
		FILE=`echo $$icon | cut -d. -f1,3`; \
		if [ "$$SIZE" = "svg" ]; then \
			SIZE="scalable"; \
			FILE="$$FILE.svg"; \
		else \
			SIZE="$${SIZE}x$${SIZE}"; \
		fi; \
		mkdir -p $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(iconsdir)/hicolor/$$SIZE/apps/$$FILE; \
	done
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk
