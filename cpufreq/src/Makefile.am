if BUILD_CPUFREQ_SELECTOR
selector_SUBDIR = cpufreq-selector
endif

SUBDIRS = $(selector_SUBDIR)

AM_CPPFLAGS = \
	-DCPUFREQ_RESOURCE_PATH=\""/org/mate/mate-applets/cpufreq/"\" \
	$(MATE_APPLETS4_CFLAGS)

libexec_PROGRAMS = mate-cpufreq-applet

BUILT_SOURCES = \
	cpufreq-resources.c		cpufreq-resources.h

nodist_mate_cpufreq_applet_SOURCES = \
	$(BUILT_SOURCES)

mate_cpufreq_applet_SOURCES = \
	cpufreq-applet.c        	cpufreq-applet.h        	\
	cpufreq-utils.c			cpufreq-utils.h			\
	cpufreq-prefs.c         	cpufreq-prefs.h         	\
	cpufreq-selector.c		cpufreq-selector.h		\
	cpufreq-popup.c                 cpufreq-popup.h			\
	cpufreq-monitor.c       	cpufreq-monitor.h	      	\
	cpufreq-monitor-factory.c	cpufreq-monitor-factory.h

if HAVE_LIBCPUFREQ
mate_cpufreq_applet_SOURCES += \
	cpufreq-monitor-libcpufreq.c \
	cpufreq-monitor-libcpufreq.h
else
mate_cpufreq_applet_SOURCES += \
	cpufreq-monitor-cpuinfo.c \
	cpufreq-monitor-cpuinfo.h \
	cpufreq-monitor-sysfs.c \
	cpufreq-monitor-sysfs.h
endif

mate_cpufreq_applet_CFLAGS =		\
	${WARN_CFLAGS}

mate_cpufreq_applet_LDADD =		\
	$(MATE_APPLETS4_LIBS)   	\
	$(LIBCPUFREQ_LIBS)

cpufreq-resources.c: $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/cpufreq/data --generate-dependencies $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/cpufreq/data --generate --c-name cpufreq $<

cpufreq-resources.h: $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/cpufreq/data --generate-dependencies $(top_srcdir)/cpufreq/data/cpufreq-resources.gresource.xml)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/cpufreq/data --generate --c-name cpufreq $<

CLEANFILES = \
	$(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
