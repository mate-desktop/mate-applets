accessx_status_iconsdir = $(iconsdir)/hicolor/48x48/apps
accessx_status_icons_DATA = mate-ax-applet.png

accessx_status_pixmapsdir  = $(pixmapsdir)/mate-accessx-status-applet
accessx_status_pixmaps_DATA =	\
	mate-ax-applet.png			\
	mate-ax-key-base.png			\
	mate-ax-key-inverse.png			\
	mate-ax-key-none.png			\
	mate-ax-key-yes.png			\
	mate-ax-key-no.png			\
	mate-mousekeys-base.png			\
	mate-mousekeys-pressed-left.png		\
	mate-mousekeys-pressed-middle.png	\
	mate-mousekeys-pressed-right.png	\
	mate-mousekeys-default-left.png		\
	mate-mousekeys-default-middle.png	\
	mate-mousekeys-default-right.png	\
	mate-sticky-shift-none.png		\
	mate-sticky-shift-latched.png		\
	mate-sticky-shift-locked.png		\
	mate-sticky-ctrl-none.png		\
	mate-sticky-ctrl-latched.png		\
	mate-sticky-ctrl-locked.png		\
	mate-sticky-alt-none.png		\
	mate-sticky-alt-latched.png		\
	mate-sticky-alt-locked.png		\
	mate-sticky-meta-latched.png		\
	mate-sticky-meta-locked.png		\
	mate-sticky-meta-none.png		\
	mate-sticky-hyper-latched.png		\
	mate-sticky-hyper-locked.png		\
	mate-sticky-hyper-none.png		\
	mate-sticky-super-latched.png		\
	mate-sticky-super-locked.png		\
	mate-sticky-super-none.png		\
	mate-ax-slowkeys.png			\
	mate-ax-slowkeys-pending.png		\
	mate-ax-slowkeys-yes.png		\
	mate-ax-slowkeys-no.png			\
	mate-ax-bouncekeys.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)/hicolor
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

EXTRA_DIST = \
	$(accessx_status_pixmaps_DATA)

-include $(top_srcdir)/git.mk
