AM_CPPFLAGS = -I$(top_srcdir)	\
	$(MATE_APPLETS4_CFLAGS)	\
	$(GIO_CFLAGS)		\
	-DGRESOURCE=\""/org/mate/mate-applets/trash/\"" \
	${WARN_CFLAGS}

trashapplet_libdir= $(pkglibdir)
trashapplet_lib_LTLIBRARIES=libtrashapplet.la

libtrashapplet_la_SOURCES = \
	trashapplet.c \
	trash-empty.h \
	trash-empty.c \
	xstuff.c \
	xstuff.h \
	trashapplet-resources.c \
	trashapplet-resources.h \
	$(NULL)

libtrashapplet_la_LIBADD = 		\
	$(MATE_APPLETS4_LIBS) 	\
	$(GIO_LIBS) \
	-lX11

trashapplet-resources.c: ../data/trashapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/trashapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name trashapplet $<

trashapplet-resources.h: ../data/trashapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../data/ --generate-dependencies $(srcdir)/../data/trashapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../data/ --generate --c-name trashapplet $<

CLEANFILES = \
	$(NULL)

-include $(top_srcdir)/git.mk
