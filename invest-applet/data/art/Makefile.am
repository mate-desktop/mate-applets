# ******************************************************************************
# Images and icon
# ******************************************************************************
icons = mate-invest-applet.16.png mate-invest-applet.22.png mate-invest-applet.48.png mate-invest-applet.svg

invest_icons = invest_up.svg invest_down.svg invest_neutral.svg

artdir = $(pkgdatadir)/invest-applet
art_DATA = $(invest_icons)

# Put icons in the hicolor theme to avoid GTK complaining
hicolor_iconsdir = $(datadir)/icons/hicolor/scalable/apps
hicolor_icons_DATA = $(invest_icons)

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
		rm -f $(DESTDIR)$(artdir)/hicolor/$$SIZE/apps/$$FILE; \
	done
	rm -f $(DESTDIR)$(hicolor_iconsdir)/$(invest_icons)
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
		mkdir -p $(DESTDIR)$(artdir)/hicolor/$$SIZE/apps/; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(artdir)/hicolor/$$SIZE/apps/$$FILE; \
	done
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = \
	$(icons) \
	$(art_DATA)


-include $(top_srcdir)/git.mk
