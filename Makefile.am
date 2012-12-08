
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

if BUILD_TIMER_APPLET
timer_applet_SUBDIR = timer-applet
endif

always_built_SUBDIRS = \
	charpick \
	drivemount \
	geyes \
	stickynotes \
	trashapplet
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
	$(timer_applet_SUBDIR)

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
	trashapplet \
	cpufreq \
	invest-applet \
	timer-applet \
	null_applet

EXTRA_DIST = \
	MAINTAINERS \
	m4/ac-define-dir.m4 \
	COPYING-DOCS \
	mate-applets.spec.in \
	mate-applets.spec \
	intltool-update.in \
	intltool-extract.in \
	intltool-merge.in \
	xmldocs.make \
	omf.make \
	mate-doc-utils.make

# We build the ChangeLog from the git log
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat --no-color --since=2009-05-23 > $@; \
	fi

dist: ChangeLog

# Make sure the ChangeLog is updated every time
.PHONY: ChangeLog

ACLOCAL_AMFLAGS = -I m4

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	mate-doc-utils.make

-include $(top_srcdir)/git.mk

