AM_CPPFLAGS = -I$(top_srcdir)	\
	$(MATE_APPLETS4_CFLAGS)	\
	$(GIO_CFLAGS)		\
	${WARN_CFLAGS}

libexec_PROGRAMS = trashapplet

BUILT_SOURCES = \
	trashapplet-resources.c \
	trashapplet-resources.h

nodist_trashapplet_SOURCES = $(BUILT_SOURCES)
trashapplet_SOURCES = \
	trashapplet.c \
	trash-empty.h \
	trash-empty.c \
	xstuff.c \
	xstuff.h

trashapplet_LDADD = 		\
	$(MATE_APPLETS4_LIBS) 	\
	$(GIO_LIBS) \
	-lX11

trashapplet-resources.c: ../trashapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../ --generate-dependencies $(srcdir)/../trashapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../ --generate --c-name trashapplet $<

trashapplet-resources.h: ../trashapplet-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/../ --generate-dependencies $(srcdir)/../trashapplet-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir)/../ --generate --c-name trashapplet $<

CLEANFILES = \
	$(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
