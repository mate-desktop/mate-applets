if BUILD_CPUFREQ_SELECTOR
selector_SUBDIR = cpufreq-selector
endif

SUBDIRS = $(selector_SUBDIR)

AM_CPPFLAGS = \
	-DCPUFREQ_MENU_UI_DIR=\""$(datadir)/mate/ui"\" 	\
	$(MATE_APPLETS4_CFLAGS) 				\
	$(MATEDESKTOP_CFLAGS) 				\
	$(DBUS_CFLAGS)

libexec_PROGRAMS = mate-cpufreq-applet

if HAVE_LIBCPUFREQ
cpufreq_files=cpufreq-monitor-libcpufreq.c    cpufreq-monitor-libcpufreq.h
endif

mate_cpufreq_applet_SOURCES =  \
	cpufreq-applet.c        	cpufreq-applet.h        	\
	cpufreq-utils.c			cpufreq-utils.h			\
	cpufreq-prefs.c         	cpufreq-prefs.h         	\
	cpufreq-selector.c		cpufreq-selector.h		\
	cpufreq-popup.c                 cpufreq-popup.h			\
	cpufreq-monitor.c       	cpufreq-monitor.h	      	\
	cpufreq-monitor-factory.c	cpufreq-monitor-factory.h	\
	cpufreq-monitor-procfs.c	cpufreq-monitor-procfs.h	\
	cpufreq-monitor-sysfs.c	        cpufreq-monitor-sysfs.h	        \
	$(cpufreq_files)						\
	cpufreq-monitor-cpuinfo.c       cpufreq-monitor-cpuinfo.h

mate_cpufreq_applet_LDADD =  \
	$(MATE_APPLETS4_LIBS)   	\
	$(MATEDESKTOP_LIBS)   	\
	$(LIBCPUFREQ_LIBS)		\
	$(DBUS_LIBS)

	

-include $(top_srcdir)/git.mk
