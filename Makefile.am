
gtop_applets = multiload

if BUILD_GTOP_APPLETS
gtop_SUBDIRS = $(gtop_applets)
endif

libmateweather_applets = \
	mateweather

if BUILD_LIBMATEWEATHER_APPLETS
libmateweather_SUBDIRS = $(libmateweather_applets)
endif

modemlights_applets = \
	modemlights

if APPLET_MODEMLIGHTS
modemlights_SUBDIRS = $(modemlights_applets)
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
#	null_applet

SUBDIRS = \
	po \
	$(always_built_SUBDIRS) \
	$(modemlights_SUBDIRS) \
	$(gtop_SUBDIRS) \
	$(libmateweather_SUBDIRS) \
	$(vfs_SUBDIRS) \
	$(battstat_SUBDIR) \
	$(accessx_status_SUBDIR) \
	$(invest_applet_SUBDIR) \
	$(cpufreq_SUBDIR) \
	$(timerapplet_SUBDIR)

DIST_SUBDIRS = \
	po \
	modemlights \
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
	null_applet

EXTRA_DIST = \
	m4/ac-define-dir.m4 \
	COPYING-DOCS \
	intltool-update.in \
	intltool-extract.in \
	intltool-merge.in \
	xmldocs.make \
	omf.make \
	autogen.sh

# We build the ChangeLog from the git log
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat --no-color --since=2009-05-23 > $@; \
	fi

dist: ChangeLog

# Make sure the ChangeLog is updated every time
.PHONY: ChangeLog

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update

-include $(top_srcdir)/git.mk

