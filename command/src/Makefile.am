NULL =

AM_CPPFLAGS =			\
	$(WARN_FLAGS)		\
	$(MATE_APPLETS4_CFLAGS)	\
	-I$(srcdir)		\
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(NULL)

BUILT_SOURCES =			\
	command-resources.c	\
	command-resources.h	\
	$(NULL)
APPLET_SOURCES =	\
	command.c	\
	ma-command.c	\
	ma-command.h	\
	$(NULL)

APPLET_LIBS =			\
	$(MATE_APPLETS4_LIBS)	\
	$(NULL)

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libcommand-applet.la
nodist_libcommand_applet_la_SOURCES = $(BUILT_SOURCES)
libcommand_applet_la_SOURCES = $(APPLET_SOURCES)
libcommand_applet_la_CFLAGS = $(AM_CFLAGS)
libcommand_applet_la_LDFLAGS = -module -avoid-version
libcommand_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = command-applet
nodist_command_applet_SOURCES = $(BUILT_SOURCES)
command_applet_SOURCES = $(APPLET_SOURCES)
command_applet_CFLAGS = $(AM_CFLAGS)
command_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

command-resources.c: $(srcdir)/../data/command-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/command-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name command $<

command-resources.h: $(srcdir)/../data/command-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/command-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name command $<

CLEANFILES =			\
	$(BUILT_SOURCES)
	$(NULL)

-include $(top_srcdir)/git.mk
