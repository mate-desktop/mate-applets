
builder_DATA = battstat_applet.ui

ACPIINC= @ACPIINC@

if NEED_LIBAPM
if HAVE_LIBAPM
APMLIB = -lapm
APMDIR =
APMINC = 
else
APMLIB = apmlib/libapm.a
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

AM_CPPFLAGS =					\
	$(MATE_APPLETS4_CFLAGS)		\
	$(MATEDESKTOP_CFLAGS)		\
	$(LIBNOTIFY_CFLAGS)			\
	$(UPOWER_CFLAGS)	\
	$(APMINC)				\
	$(ACPIINC)				\
	-DG_LOG_DOMAIN=\"battstat_applet\"	\
	-DBATTSTAT_MENU_UI_DIR=\""$(uidir)"\"


libexec_PROGRAMS = battstat-applet


battstat_applet_SOURCES = \
	battstat.h \
	properties.c \
	battstat_applet.c \
	power-management.c \
	acpi-linux.c \
	acpi-linux.h \
	acpi-freebsd.c \
	acpi-freebsd.h \
	battstat-upower.c \
	battstat-upower.h


battstat_applet_LDADD = \
		     $(MATE_APPLETS4_LIBS) \
		     $(MATEDESKTOP_LIBS) \
		     $(LIBNOTIFY_LIBS) \
			 $(APMLIB) \
			 $(UPOWER_LIBS) \
			 -lm

batstat_gschema_in_files = org.mate.panel.applet.battstat.gschema.xml.in
gsettings_SCHEMAS = $(batstat_gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

appletdir       = $(datadir)/mate-panel/applets
applet_in_files = org.mate.applets.BattstatApplet.mate-panel-applet.desktop.in
applet_DATA     = $(applet_in_files:.mate-panel-applet.desktop.in=.mate-panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

$(applet_DATA): $(applet_in_files) Makefile
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.mate.panel.applet.BattstatAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.mate.panel.applet.BattstatAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

CLEANFILES = $(applet_DATA) $(applet_in_files) $(service_DATA) $(gsettings_SCHEMAS) *.gschema.valid

uidir = $(datadir)/mate/ui
ui_DATA = battstat-applet-menu.xml

EXTRA_DIST = 							\
	$(builder_DATA)						\
	$(ui_DATA)						\
	$(applet_in_files).in	\
	$(service_in_files)					\
	$(batstat_gschema_in_files)


-include $(top_srcdir)/git.mk
