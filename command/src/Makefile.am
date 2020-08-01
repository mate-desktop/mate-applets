NULL =

AM_CPPFLAGS =			\
	$(MATE_APPLETS4_CFLAGS)	\
	-I$(srcdir)		\
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(NULL)

libexec_PROGRAMS = command-applet

BUILT_SOURCES =			\
	command-resources.c	\
	command-resources.h	\
	$(NULL)

nodist_command_applet_SOURCES = \
	$(BUILT_SOURCES)	\
	$(NULL)

command_applet_SOURCES =	\
	command.c		\
	ma-command.c		\
	ma-command.h		\
	$(NULL)

command_applet_LDADD =		\
	$(MATE_APPLETS4_LIBS)	\
	$(NULL)

command_applet_CFLAGS =		\
	$(WARN_CFLAGS)		\
	$(NULL)

command-resources.c: $(srcdir)/../data/command-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/command-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name command $<

command-resources.h: $(srcdir)/../data/command-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/command-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name command $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
