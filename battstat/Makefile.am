NULL =

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

applet_in_files = org.mate.applets.BattstatApplet.mate-panel-applet.desktop.in
service_in_files = org.mate.panel.applet.BattstatAppletFactory.service.in
gschema_in_files = org.mate.panel.applet.battstat.gschema.xml.in

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

BUILT_SOURCES =			\
	battstat-resources.c	\
	battstat-resources.h	\
	$(NULL)
APPLET_SOURCES =		\
	battstat.h		\
	battstat_applet.c	\
	battstat-preferences.c	\
	battstat-preferences.h	\
	power-management.c	\
	acpi-linux.c		\
	acpi-linux.h		\
	acpi-freebsd.c		\
	acpi-freebsd.h		\
	battstat-upower.c	\
	battstat-upower.h	\
	$(NULL)

APPLET_LIBS =			\
	$(MATE_APPLETS4_LIBS)	\
	$(LIBNOTIFY_LIBS)	\
	$(APMLIB)		\
	$(UPOWER_LIBS)		\
	-lm			\
	$(NULL)

if ENABLE_IN_PROCESS
APPLET_LOCATION = $(pkglibdir)/libmate-battstat-applet.so

pkglib_LTLIBRARIES = libmate-battstat-applet.la
nodist_libmate_battstat_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_battstat_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_battstat_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_battstat_applet_la_LDFLAGS = -module -avoid-version
libmate_battstat_applet_la_LIBADD = $(APPLET_LIBS)

else !ENABLE_IN_PROCESS
APPLET_LOCATION = $(libexecdir)/battstat-applet

libexec_PROGRAMS = battstat-applet
nodist_battstat_applet_SOURCES = $(BUILT_SOURCES)
battstat_applet_SOURCES = $(APPLET_SOURCES)
battstat_applet_CFLAGS = $(AM_CFLAGS)
battstat_applet_LDADD = $(APPLET_LIBS)

servicedir = $(datadir)/dbus-1/services
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN)sed \
	    -e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
	    $< > $@
endif !ENABLE_IN_PROCESS

battstat-resources.c: battstat-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/battstat-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name battstat $<

battstat-resources.h: battstat-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/battstat-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name battstat $<

gsettings_SCHEMAS = $(gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

appletdir       = $(datadir)/mate-panel/applets
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@APPLET_LOCATION\@|$(APPLET_LOCATION)|" \
            -e "s|\@APPLET_IN_PROCESS\@|$(APPLET_IN_PROCESS)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@


CLEANFILES =			\
	$(applet_DATA)		\
	$(applet_in_files)	\
	$(service_DATA)		\
	$(gsettings_SCHEMAS)	\
	$(BUILT_SOURCES)	\
	*.gschema.valid		\
	$(NULL)

EXTRA_DIST =					\
	battstat-applet-menu.xml		\
	battstat-preferences.ui			\
	battstat-resources.gresource.xml	\
	$(applet_in_files).in			\
	$(service_in_files)			\
	$(gschema_in_files)			\
	$(NULL)

-include $(top_srcdir)/git.mk
