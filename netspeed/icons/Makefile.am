NULL =

iconsdir = $(datadir)/icons/hicolor

png_icons = \
	16x16/apps/mate-netspeed-applet.png \
	16x16/devices/mate-netspeed-plip.png \
	22x22/apps/mate-netspeed-applet.png \
	22x22/devices/mate-netspeed-plip.png \
	24x24/apps/mate-netspeed-applet.png \
	24x24/devices/mate-netspeed-plip.png \
	24x24/status/mate-netspeed-wireless-25.png \
	24x24/status/mate-netspeed-wireless-50.png \
	24x24/status/mate-netspeed-wireless-75.png \
	24x24/status/mate-netspeed-wireless-100.png \
	32x32/apps/mate-netspeed-applet.png \
	32x32/devices/mate-netspeed-plip.png \
	48x48/apps/mate-netspeed-applet.png \
	48x48/devices/mate-netspeed-plip.png \
	$(NULL)

nobase_dist_icons_DATA = \
	$(png_icons) \
	scalable/apps/mate-netspeed-applet.svg \
	$(NULL)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)

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

-include $(top_srcdir)/git.mk
