# OMNIBOOK plugin
plugindir = $(libdir)/mate-sensors-applet/plugins

AM_CPPFLAGS = -DMATELOCALEDIR=\""$(datadir)/locale/"\" \
	-DG_LOG_DOMAIN=\""sensors-applet"\" \
	-DPIXMAPS_DIR=\""$(datadir)/pixmaps/$(PACKAGE)/"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DPREFIX=\""$(prefix)"\" \
	-I$(top_srcdir)/sensors-applet \
	$(GLIB_CFLAGS) \
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libomnibook.la

libomnibook_la_SOURCES =	\
	omnibook-plugin.h	\
	omnibook-plugin.c

libomnibook_la_LDFLAGS = $(PLUGIN_LIBTOOL_FLAGS) $(GLIB_LIBS)
libomnibook_la_LIBADD = $(top_builddir)/sensors-applet/lib/libmate-sensors-applet-plugin.la

-include $(top_srcdir)/git.mk
