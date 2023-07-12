NULL =

AM_CPPFLAGS =				\
	-I.				\
	-I$(srcdir)			\
	-DDRIVEMOUNT_RESOURCE_PATH=\""/org/mate/mate-applets/drivemount/"\" \
	${WARN_CFLAGS}			\
	$(MATE_APPLETS4_CFLAGS)		\
	$(NULL)

BUILT_SOURCES =				\
	drivemount-resources.c		\
	drivemount-resources.h		\
	$(NULL)
APPLET_SOURCES =			\
	drivemount.c			\
	drive-list.c			\
	drive-list.h			\
	drive-button.c			\
	drive-button.h			\
	$(NULL)

APPLET_LIBS = $(MATE_APPLETS4_LIBS)

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmate-drivemount-applet.la
nodist_libmate_drivemount_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_drivemount_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_drivemount_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_drivemount_applet_la_LDFLAGS = -module -avoid-version
libmate_drivemount_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = mate-drivemount-applet
nodist_mate_drivemount_applet_SOURCES = $(BUILT_SOURCES)
mate_drivemount_applet_SOURCES = $(APPLET_SOURCES)
mate_drivemount_applet_CFLAGS = $(AM_CFLAGS)
mate_drivemount_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

drivemount-resources.c: ../data/drivemount-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/drivemount-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name drivemount $<

drivemount-resources.h: ../data/drivemount-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/drivemount-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name drivemount $<

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(NULL)

-include $(top_srcdir)/git.mk
