# Makefile.am
#
# Author: Jörgen Pehrson <jp@spektr.eu.org>
# Created: Thu Jun 15 23:08:41 CEST 2000
#

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/apmlib			\
	$(MATE_APPLETS_CFLAGS)			\
	-DG_LOG_DOMAIN=\"battstat_applet\"

noinst_LTLIBRARIES = libapm.la

libapm_la_SOURCES = apmlib.c apm.h


-include $(top_srcdir)/git.mk
