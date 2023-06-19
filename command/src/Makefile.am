NULL =

command_applet_libdir= $(pkglibdir)
command_applet_lib_LTLIBRARIES=libcommand-applet.la

AM_CPPFLAGS =			\
	$(MATE_APPLETS4_CFLAGS)	\
	-I$(srcdir)		\
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(NULL)


libcommand_applet_la_SOURCES =	\
	command.c		\
	ma-command.c		\
	ma-command.h		\
	command-resources.c	\
	command-resources.h	\
	$(NULL)

libcommand_applet_la_LIBADD =		\
	$(MATE_APPLETS4_LIBS)	\
	$(NULL)

command-resources.c: $(srcdir)/../data/command-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/command-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name command $<

command-resources.h: $(srcdir)/../data/command-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data --generate-dependencies $(srcdir)/../data/command-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data --generate --c-name command $<

CLEANFILES =			\
	$(NULL)

-include $(top_srcdir)/git.mk
