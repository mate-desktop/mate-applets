
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
	command \
	netspeed

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-compile-warnings=no \
	CFLAGS='-Wno-deprecated-declarations'

M4_FILES = \
	m4/ac-define-dir.m4 \
	m4/as-ac-expand.m4

EXTRA_DIST = \
	$(M4_FILES) \
	COPYING-DOCS \
	xmldocs.make \
	omf.make \
	autogen.sh

MAINTAINERCLEANFILES = \
	ABOUT-NLS \
	INSTALL \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	config.h.in~ \
	config.log \
	config.rpath \
	config.sub \
	configure \
	depcomp \
	install-sh \
	libtool \
	ltmain.sh \
	missing \
	omf.make \
	po/*.po~ \
	po/@GETTEXT_PACKAGE@.pot \
	po/Makefile.in.in \
	po/Makevars.template \
	po/Rules-quot \
	po/boldquot.sed \
	po/en@boldquot.header \
	po/en@quot.header \
	po/insert-header.sin \
	po/quot.sed \
	po/remove-potcdate.sin \
	stamp-h1 \
	xmldocs.make \
	$(filter-out $(M4_FILES), $(wildcard m4/*.m4)) \
	`find "$(srcdir)" -type f -name Makefile.in -print`

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCLEANFILES =

-include $(top_srcdir)/git.mk

