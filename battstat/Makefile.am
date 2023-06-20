NULL =

APPLET_LOCATION = $(libdir)/mate-applets/libmate-battstat-applet.so

ACPIINC= @ACPIINC@

if NEED_LIBAPM
if HAVE_LIBAPM
APMLIB = -lapm
APMDIR =
APMINC =
else
APMLIB = apmlib/libapm.la
APMDIR = apmlib
APMINC = -I$(srcdir)/apmlib/
endif
else
APMLIB =
APMDIR =
APMINC =
endif

SUBDIRS = docs sounds $(APMDIR)

DIST_SUBDIRS = docs sounds apmlib

mate_battstat_applet_libdir= $(pkglibdir)
mate_battstat_applet_lib_LTLIBRARIES=libmate-battstat-applet.la

AM_CPPFLAGS =					\
	${WARN_CFLAGS}				\
	$(MATE_APPLETS4_CFLAGS)			\
	$(LIBNOTIFY_CFLAGS)			\
	$(UPOWER_CFLAGS)			\
	$(APMINC)				\
	$(ACPIINC)				\
	-DG_LOG_DOMAIN=\"battstat_applet\"	\
	-DBATTSTAT_RESOURCE_PATH=\""/org/mate/mate-applets/battstat/"\" \
	$(NULL)


libmate_battstat_applet_la_SOURCES =	\
	battstat.h		\
	battstat_applet.c	\
	battstat-preferences.c  \
	battstat-preferences.h	\
	power-management.c	\
	acpi-linux.c		\
	acpi-linux.h		\
	acpi-freebsd.c		\
	acpi-freebsd.h		\
	battstat-upower.c	\
	battstat-upower.h	\
	battstat-resources.c \
	battstat-resources.h  \
	$(NULL)

libmate_battstat_applet_la_LIBADD =		\
	$(MATE_APPLETS4_LIBS)	\
	$(LIBNOTIFY_LIBS)	\
	$(APMLIB)		\
	$(UPOWER_LIBS)		\
	-lm			\
	$(NULL)

battstat-resources.c: battstat-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/battstat-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name battstat $<

battstat-resources.h: battstat-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/battstat-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name battstat $<

batstat_gschema_in_files = org.mate.panel.applet.battstat.gschema.xml.in
gsettings_SCHEMAS = $(batstat_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.BattstatApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@


CLEANFILES =			\
	$(applet_DATA)		\
	$(applet_in_files)	\
	$(gsettings_SCHEMAS)	\
	$(BUILT_SOURCES)	\
	*.gschema.valid		\
	$(NULL)

EXTRA_DIST =					\
	battstat-applet-menu.xml		\
	battstat-preferences.ui			\
	battstat-resources.gresource.xml	\
	$(applet_in_files).in			\
	$(batstat_gschema_in_files)		\
	$(NULL)

-include $(top_srcdir)/git.mk
