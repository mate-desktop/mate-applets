if WITH_AYATANA_INDICATOR
INDICATOR_CFLAGS = $(AYATANA_INDICATOR_CFLAGS)		\
                   $(AYATANA_INDICATOR_NG_CFLAGS)
INDICATOR_LIBS   = $(AYATANA_INDICATOR_LIBS)		\
                   $(AYATANA_INDICATOR_NG_LIBS)
endif

if WITH_UBUNTU_INDICATOR
INDICATOR_CFLAGS = $(UBUNTU_INDICATOR_CFLAGS)		\
                   $(UBUNTU_INDICATOR_NG_CFLAGS)
INDICATOR_LIBS   = $(UBUNTU_INDICATOR_LIBS)		\
                   $(UBUNTU_INDICATOR_NG_LIBS)
endif

libexec_PROGRAMS = \
	mate-indicator-applet \
	mate-indicator-applet-appmenu \
	mate-indicator-applet-complete

mate_indicator_applet_CFLAGS = \
	-DG_LOG_DOMAIN=\""Indicator-Applet"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DINDICATOR_DIR=\""$(INDICATORDIR)"\" \
	-DINDICATOR_ICONS_DIR=\""$(INDICATORICONSDIR)"\" \
	-DINDICATOR_APPLET \
	-I$(srcdir)/.. \
	$(INDICATOR_APPLET_CFLAGS) \
	$(INDICATOR_CFLAGS) \
	$(WARN_CFLAGS)

mate_indicator_applet_SOURCES = \
	applet-main.c \
	eggaccelerators.c \
	eggaccelerators.h \
	tomboykeybinder.c \
	tomboykeybinder.h

mate_indicator_applet_LDADD = \
	$(INDICATOR_APPLET_LIBS) \
	$(INDICATOR_LIBS) \
	-lX11

mate_indicator_applet_appmenu_CFLAGS = \
	-DG_LOG_DOMAIN=\""Indicator-Applet-Appmenu"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DINDICATOR_DIR=\""$(INDICATORDIR)"\" \
	-DINDICATOR_ICONS_DIR=\""$(INDICATORICONSDIR)"\" \
	-DINDICATOR_APPLET_APPMENU \
	-I$(srcdir)/.. \
	$(INDICATOR_APPLET_CFLAGS) \
	$(INDICATOR_CFLAGS) \
	$(WARN_CFLAGS)

mate_indicator_applet_appmenu_SOURCES = \
	applet-main.c \
	eggaccelerators.c \
	eggaccelerators.h \
	tomboykeybinder.c \
	tomboykeybinder.h

mate_indicator_applet_appmenu_LDADD = \
	$(INDICATOR_APPLET_LIBS) \
	$(INDICATOR_LIBS) \
	-lX11

mate_indicator_applet_complete_CFLAGS = \
	-DG_LOG_DOMAIN=\""Indicator-Applet-Complete"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DINDICATOR_DIR=\""$(INDICATORDIR)"\" \
	-DINDICATOR_ICONS_DIR=\""$(INDICATORICONSDIR)"\" \
	-DINDICATOR_APPLET_COMPLETE \
	-I$(srcdir)/.. \
	$(INDICATOR_APPLET_CFLAGS) \
	$(INDICATOR_CFLAGS) \
	$(WARN_CFLAGS)

mate_indicator_applet_complete_SOURCES =	\
	applet-main.c \
	eggaccelerators.c \
	eggaccelerators.h \
	tomboykeybinder.c \
	tomboykeybinder.h

mate_indicator_applet_complete_LDADD = \
	$(INDICATOR_APPLET_LIBS) \
	$(INDICATOR_LIBS) \
	-lX11

-include $(top_srcdir)/git.mk
