AM_CPPFLAGS = \
	$(WARN_FLAGS) \
	$(MATE_APPLETS4_CFLAGS) \
	-I$(srcdir) \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(NULL)

APPLET_SOURCES = hwmonitor.c hwmonitor-sensors.c hwmonitor-sensors.h
APPLET_LIBS = $(MATE_APPLETS4_LIBS)

if ENABLE_IN_PROCESS
pkglib_LTLIBRARIES = libhwmonitor-applet.la
libhwmonitor_applet_la_SOURCES = $(APPLET_SOURCES)
libhwmonitor_applet_la_CFLAGS = $(AM_CFLAGS)
libhwmonitor_applet_la_LDFLAGS = -module -avoid-version
libhwmonitor_applet_la_LIBADD = $(APPLET_LIBS)
else !ENABLE_IN_PROCESS
libexec_PROGRAMS = hwmonitor-applet
hwmonitor_applet_SOURCES = $(APPLET_SOURCES)
hwmonitor_applet_CFLAGS = $(AM_CFLAGS)
hwmonitor_applet_LDADD = $(APPLET_LIBS)
endif !ENABLE_IN_PROCESS

check_PROGRAMS = test-hwmonitor-sensors
TESTS = $(check_PROGRAMS)
test_hwmonitor_sensors_SOURCES = test-hwmonitor-sensors.c hwmonitor-sensors.c hwmonitor-sensors.h
test_hwmonitor_sensors_CFLAGS = $(AM_CFLAGS) $(MATE_APPLETS4_CFLAGS)
test_hwmonitor_sensors_LDADD = $(MATE_APPLETS4_LIBS)

-include $(top_srcdir)/git.mk
