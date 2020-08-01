iconsdir = $(datadir)/icons/hicolor

png_icons = \
	16x16/apps/mate-eyes-applet.png \
	22x22/apps/mate-eyes-applet.png \
	24x24/apps/mate-eyes-applet.png \
	32x32/apps/mate-eyes-applet.png \
	64x64/apps/mate-eyes-applet.png \
	256x256/apps/mate-eyes-applet.png \
	$(NULL)

nobase_dist_icons_DATA = \
	$(png_icons) \
	scalable/apps/mate-eyes-applet.svg \
	$(NULL)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk
