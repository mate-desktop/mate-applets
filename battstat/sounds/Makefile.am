
soundlistdir = $(sysconfdir)/sound/events
soundlist_in_files = mate-battstat_applet.soundlist.desktop.in
soundlist_DATA = $(soundlist_in_files:.soundlist.desktop.in=.soundlist)

$(soundlist_DATA): $(soundlist_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=description --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(soundlist_DATA) $(soundlist_in_files)

CLEANFILES = $(soundlist_DATA)

-include $(top_srcdir)/git.mk
