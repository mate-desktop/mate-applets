@YELP_HELP_RULES@

HELP_ID = mate-drivemount
HELP_FILES = index.docbook legal.xml

HELP_MEDIA = figures/drivemount-applet_eject.png \
		figures/drivemount-applet_example.png \
		figures/drivemount-applet_mount.png \
		figures/drivemount-applet_open.png \
		figures/drivemount-applet_status.png

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

-include $(top_srcdir)/git.mk
