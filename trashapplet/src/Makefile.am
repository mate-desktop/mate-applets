AM_CPPFLAGS = -I$(top_srcdir)		\
	   $(MATE_APPLETS4_CFLAGS)	\
	   $(GIO_CFLAGS)		\
           -DTRASH_MENU_UI_DIR=\""$(datadir)/mate-2.0/ui"\"

libexec_PROGRAMS = trashapplet

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

-include $(top_srcdir)/git.mk
