##############################
# Applets
##############################

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = \
	org.mate.applets.Indicator.mate-panel-applet.desktop.in \
	org.mate.applets.IndicatorComplete.mate-panel-applet.desktop.in \
	org.mate.applets.IndicatorAppmenu.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

%.mate-panel-applet.desktop.in: %.mate-panel-applet.desktop.in.in Makefile
	$(AM_V_GEN)sed \
	    -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
	    $< > $@

%.mate-panel-applet: %.mate-panel-applet.desktop.in Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

##############################
# DBus Services
##############################

servicedir       = $(datadir)/dbus-1/services
service_in_files = \
	org.mate.panel.applet.IndicatorAppletFactory.service.in \
	org.mate.panel.applet.IndicatorAppletCompleteFactory.service.in \
	org.mate.panel.applet.IndicatorAppletAppmenuFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

%.service: %.service.in Makefile
	$(AM_V_GEN)sed \
	    -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
	    $< > $@

##############################
# Icons
##############################

iconsdir = $(datadir)/icons/hicolor/scalable/apps

icons_DATA = mate-indicator-applet.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi


##############################
# Autojunk
##############################

CLEANFILES =					\
	$(applet_in_files)			\
	$(applet_DATA)				\
	$(service_DATA)				\
	*.bak

EXTRA_DIST =					\
	$(icons_DATA)				\
	org.mate.applets.Indicator.mate-panel-applet.desktop.in.in \
	org.mate.applets.IndicatorComplete.mate-panel-applet.desktop.in.in \
	org.mate.applets.IndicatorAppmenu.mate-panel-applet.desktop.in.in	\
	$(service_in_files)

MAINTAINERCLEANFILES =				\
	Makefile.in

-include $(top_srcdir)/git.mk
