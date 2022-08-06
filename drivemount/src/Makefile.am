NULL =

AM_CPPFLAGS =				\
	-I.				\
	-I$(srcdir)			\
	-DDRIVEMOUNT_RESOURCE_PATH=\""/org/mate/mate-applets/drivemount/"\" \
	${WARN_CFLAGS}			\
	$(MATE_APPLETS4_CFLAGS)		\
	$(NULL)

libexec_PROGRAMS = mate-drivemount-applet

BUILT_SOURCES =				\
	drivemount-resources.c		\
	drivemount-resources.h		\
	$(NULL)

nodist_mate_drivemount_applet_SOURCES =	\
	$(BUILT_SOURCES)		\
	$(NULL)

mate_drivemount_applet_SOURCES =	\
	drivemount.c			\
	drive-list.c			\
	drive-list.h			\
	drive-button.c			\
	drive-button.h			\
	$(NULL)

mate_drivemount_applet_LDADD =		\
	$(MATE_APPLETS4_LIBS)

drivemount-resources.c: ../data/drivemount-resources.gresource.xml `$(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/drivemount-resources.gresource.xml`
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name drivemount $<

drivemount-resources.h: ../data/drivemount-resources.gresource.xml `$(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/drivemount-resources.gresource.xml`
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name drivemount $<

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(NULL)

-include $(top_srcdir)/git.mk
