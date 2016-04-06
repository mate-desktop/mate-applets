
gtop_applets = multiload netspeed

if BUILD_GTOP_APPLETS
gtop_SUBDIRS = $(gtop_applets)
endif

libmateweather_applets = \
	mateweather

if BUILD_LIBMATEWEATHER_APPLETS
libmateweather_SUBDIRS = $(libmateweather_applets)
endif

if BUILD_BATTSTAT_APPLET
battstat_SUBDIR = battstat
endif

if APPLET_ACCESSX
accessx_status_SUBDIR = accessx-status
endif

if BUILD_CPUFREQ_APPLET
cpufreq_SUBDIR = cpufreq
endif

if BUILD_INVEST_APPLET
invest_applet_SUBDIR = invest-applet
endif

if BUILD_TIMERAPPLET
timerapplet_SUBDIR = timerapplet
endif

always_built_SUBDIRS = \
	charpick \
	drivemount \
	geyes \
	stickynotes \
	trashapplet \
	command

SUBDIRS = \
	po \
	$(always_built_SUBDIRS) \
	$(gtop_SUBDIRS) \
	$(libmateweather_SUBDIRS) \
	man	\
	$(battstat_SUBDIR) \
	$(accessx_status_SUBDIR) \
	$(invest_applet_SUBDIR) \
	$(cpufreq_SUBDIR) \
	$(timerapplet_SUBDIR)

DIST_SUBDIRS = \
	po \
	drivemount \
	charpick \
	geyes \
	battstat \
	multiload \
	mateweather \
	man \
	accessx-status \
	stickynotes \
	timerapplet \
	trashapplet \
	cpufreq \
	invest-applet \
	command \
	netspeed

EXTRA_DIST = \
	m4/ac-define-dir.m4 \
	COPYING-DOCS \
	intltool-update.in \
	intltool-extract.in \
	intltool-merge.in \
	xmldocs.make \
	omf.make \
	autogen.sh

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update

-include $(top_srcdir)/git.mk

