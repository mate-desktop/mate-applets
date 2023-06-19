NULL =

mate_drivemount_applet_libdir= $(pkglibdir)
mate_drivemount_applet_lib_LTLIBRARIES=libmate-drivemount-applet.la

AM_CPPFLAGS =				\
	-I.				\
	-I$(srcdir)			\
	-DDRIVEMOUNT_RESOURCE_PATH=\""/org/mate/mate-applets/drivemount/"\" \
	${WARN_CFLAGS}			\
	$(MATE_APPLETS4_CFLAGS)		\
	$(NULL)

libmate_drivemount_applet_la_SOURCES =	\
	drivemount.c			\
	drive-list.c			\
	drive-list.h			\
	drive-button.c			\
	drive-button.h			\
	drivemount-resources.c		\
	drivemount-resources.h		\
	$(NULL)

libmate_drivemount_applet_la_LIBADD =		\
	$(MATE_APPLETS4_LIBS)

drivemount-resources.c: ../data/drivemount-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/drivemount-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name drivemount $<

drivemount-resources.h: ../data/drivemount-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/drivemount-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name drivemount $<

CLEANFILES =				\
	$(NULL)

-include $(top_srcdir)/git.mk
