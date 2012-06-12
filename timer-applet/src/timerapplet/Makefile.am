SUBDIRS = controllers core ui

defs.py: defs.py.in Makefile
	sed -e "s|\@PACKAGE\@|$(PACKAGE)|" \
	    -e "s|\@VERSION\@|$(VERSION)|" \
	    -e "s|\@RESOURCESDIR\@|$(pkgdatadir)|" \
	    -e "s|\@IMAGESDIR\@|$(datadir)/pixmaps|" \
	    -e "s|\@LOCALEDIR\@|$(localedir)|" \
	    -e "s|\@GETTEXT_PACKAGE\@|$(GETTEXT_PACKAGE)|" $< > $@

EXTRA_DIST = \
	defs.py.in

CLEANFILES = \
	defs.py

# Need this so that defs.py is actually created after cleaning.
BUILT_SOURCES = defs.py

moduledir = $(pythondir)/timerapplet
module_PYTHON = \
	__init__.py \
	config.py \
	defs.py \
	utils.py \
	logger.py

