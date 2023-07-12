if BUILD_CPUFREQ_SELECTOR
selector_SUBDIR = cpufreq-selector
endif

SUBDIRS = $(selector_SUBDIR)

AM_CPPFLAGS = \
	-DCPUFREQ_RESOURCE_PATH=\""/org/mate/mate-applets/cpufreq/"\" \
	$(NULL)

AM_CFLAGS =				\
	$(MATE_APPLETS4_CFLAGS)		\
	$(GIO_CFLAGS) \
	$(WARN_CFLAGS) \
	$(NULL)

BUILT_SOURCES = 		\
	cpufreq-resources.c	\
	cpufreq-resources.h	\
	$(NULL)
APPLET_SOURCES =			\
	cpufreq-applet.c		\
	cpufreq-applet.h		\
	cpufreq-utils.c			\
	cpufreq-utils.h			\
	cpufreq-prefs.c			\
	cpufreq-prefs.h			\
	cpufreq-selector.c		\
	cpufreq-selector.h		\
	cpufreq-popup.c			\
	cpufreq-popup.h			\
	cpufreq-monitor.c		\
	cpufreq-monitor.h		\
	cpufreq-monitor-factory.c	\
	cpufreq-monitor-factory.h	\
	$(NULL)

if HAVE_LIBCPUFREQ
APPLET_SOURCES +=			\
	cpufreq-monitor-libcpufreq.c	\
	cpufreq-monitor-libcpufreq.h	\
	$(NULL)
else
APPLET_SOURCES +=			\
	cpufreq-monitor-cpuinfo.c	\
	cpufreq-monitor-cpuinfo.h	\
	cpufreq-monitor-sysfs.c		\
	cpufreq-monitor-sysfs.h		\
	$(NULL)
endif

APPLET_LIBS = \
	$(MATE_APPLETS4_LIBS)		\
	$(LIBCPUFREQ_LIBS) \
	$(NULL)


if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libmate-cpufreq-applet.la
nodist_libmate_cpufreq_applet_la_SOURCES = $(BUILT_SOURCES)
libmate_cpufreq_applet_la_SOURCES = $(APPLET_SOURCES)
libmate_cpufreq_applet_la_CFLAGS = $(AM_CFLAGS)
libmate_cpufreq_applet_la_LDFLAGS = -module -avoid-version
libmate_cpufreq_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = mate-cpufreq-applet
nodist_mate_cpufreq_applet_SOURCES = $(BUILT_SOURCES)
mate_cpufreq_applet_SOURCES = $(APPLET_SOURCES)
mate_cpufreq_applet_CFLAGS = $(AM_CFLAGS)
mate_cpufreq_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

cpufreq-resources.c: $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/cpufreq/data --generate-dependencies $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/cpufreq/data --generate --c-name cpufreq $<

cpufreq-resources.h: $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/cpufreq/data --generate-dependencies $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/cpufreq/data --generate --c-name cpufreq $<

CLEANFILES = \
	$(BUILT_SOURCES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
