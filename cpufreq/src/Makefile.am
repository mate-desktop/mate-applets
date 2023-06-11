if BUILD_CPUFREQ_SELECTOR
selector_SUBDIR = cpufreq-selector
endif

SUBDIRS = $(selector_SUBDIR)

AM_CPPFLAGS = \
	$(MATE_APPLETS4_CFLAGS)

cpufreq_libdir= $(pkglibdir)
cpufreq_lib_LTLIBRARIES=libmatecpufreqapplet.la

libmatecpufreqapplet_la_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-DGTK_BUILDERDIR=\""$(pkgdatadir)/builder"\" \
	-DCPUFREQ_RESOURCE_PATH=\""/org/mate/mate-applets/cpufreq/"\" \
	$(NULL)

libmatecpufreqapplet_la_CFLAGS = \
	$(MATE_APPLETS4_CFLAGS)		\
	$(GIO_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

libmatecpufreqapplet_la_SOURCES = \
	cpufreq-applet.c        \
    cpufreq-applet.h        \
	cpufreq-utils.c			\
    cpufreq-utils.h			\
	cpufreq-prefs.c         \
    cpufreq-prefs.h         \
	cpufreq-selector.c		\
    cpufreq-selector.h		\
	cpufreq-popup.c         \
    cpufreq-popup.h			\
	cpufreq-monitor.c       \
    cpufreq-monitor.h	    \
	cpufreq-monitor-factory.c	\
    cpufreq-monitor-factory.h   \
    cpufreq-resources.c     \
    cpufreq-resources.h    \
	$(NULL)

libmatecpufreqapplet_la_LDFLAGS = \
	-module -avoid-version \
	$(WARN_LDFLAGS) \
	$(AM_LDFLAGS) \
	$(NULL)

libmatecpufreqapplet_la_LIBADD = \
	$(MATE_APPLETS4_LIBS)		\
	$(LIBCPUFREQ_LIBS) \
	$(NULL)

if HAVE_LIBCPUFREQ
libmatecpufreqapplet_la_SOURCES += \
	cpufreq-monitor-libcpufreq.c \
	cpufreq-monitor-libcpufreq.h \
	$(NULL)
else
libmatecpufreqapplet_la_SOURCES += \
	cpufreq-monitor-cpuinfo.c \
	cpufreq-monitor-cpuinfo.h \
	cpufreq-monitor-sysfs.c \
	cpufreq-monitor-sysfs.h \
	$(NULL)
endif

cpufreq-resources.c: $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/cpufreq/data --generate-dependencies $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/cpufreq/data --generate --c-name cpufreq $<

cpufreq-resources.h: $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/cpufreq/data --generate-dependencies $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/cpufreq/data --generate --c-name cpufreq $<

CLEANFILES = \
	cpufreq-resources.c		\
	cpufreq-resources.h     \
	$(NULL)

-include $(top_srcdir)/git.mk
