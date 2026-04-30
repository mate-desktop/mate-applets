pixmapsdir = $(datadir)/pixmaps/mate-sensors-applet
pixmaps_DATA = \
	very-low-temp-icon.png \
	low-temp-icon.png \
	normal-temp-icon.png \
	high-temp-icon.png \
	very-high-temp-icon.png

deviceiconsdir = $(datadir)/icons/hicolor/22x22/devices
deviceicons_DATA = \
	mate-sensors-applet-battery.png \
	mate-sensors-applet-case.png \
	mate-sensors-applet-chip.png \
	mate-sensors-applet-cpu.png \
	mate-sensors-applet-gpu.png \
	mate-sensors-applet-fan.png \
	mate-sensors-applet-drive-harddisk.png \
	mate-sensors-applet-memory.png

appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = mate-sensors-applet.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update_icon_cache
uninstall-hook: update_icon_cache
update_icon_cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = \
	$(pixmaps_DATA) \
	$(deviceicons_DATA) \
	$(appicon_DATA)

-include $(top_srcdir)/git.mk
