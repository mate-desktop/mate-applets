AM_CPPFLAGS = -I$(top_srcdir)	\
	$(MATE_APPLETS4_CFLAGS)	\
	$(GIO_CFLAGS)		\
	-DGRESOURCE=\""/org/mate/mate-applets/trash/\"" \
	${WARN_CFLAGS}

BUILT_SOURCES =			\
	trashapplet-resources.c	\
	trashapplet-resources.h	\
	$(NULL)
APPLET_SOURCES =	\
	trashapplet.c	\
	trash-empty.h	\
	trash-empty.c	\
	$(NULL)

APPLET_LIBS = 			\
	$(MATE_APPLETS4_LIBS)	\
	$(GIO_LIBS)		\
	-lX11

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmate-trash-applet.la
nodist_libmate_trash_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_trash_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_trash_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_trash_applet_la_LDFLAGS = -module -avoid-version
libmate_trash_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = trashapplet
nodist_trashapplet_SOURCES = $(BUILT_SOURCES)
trashapplet_SOURCES = $(APPLET_SOURCES)
trashapplet_CFLAGS = $(AM_CFLAGS)
trashapplet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

trashapplet-resources.c: ../data/trashapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/trashapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name trashapplet $<

trashapplet-resources.h: ../data/trashapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/trashapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name trashapplet $<

CLEANFILES =			\
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
