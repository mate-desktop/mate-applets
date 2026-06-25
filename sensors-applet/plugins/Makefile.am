NULL=

DIST_SUBDIRS = \
	acpi \
	dummy \
	udisks2 \
	hddtemp \
        i2c-proc \
        i2c-sys \
	i8k \
	ibm-acpi \
        libsensors \
	mbmon \
	netbsd \
        nvidia \
	omnibook \
	pmu-sys \
	smu-sys \
	sonypi \
	$(NULL)

SUBDIRS = \
	acpi \
	hddtemp \
	i8k \
	ibm-acpi \
	mbmon \
	omnibook \
	pmu-sys \
	smu-sys \
	sonypi

if DUMMYPLUGIN
SUBDIRS	     += dummy
endif

if UDISKS2
SUBDIRS	     += udisks2
endif

if LIBSENSORS
SUBDIRS      += libsensors
else
SUBDIRS      += i2c-proc i2c-sys
endif

if NETBSD
SUBDIRS      += netbsd
endif

if NVIDIA
SUBDIRS      += nvidia
endif

-include $(top_srcdir)/git.mk
