@YELP_HELP_RULES@

HELP_ID = mate-cpufreq-applet
HELP_FILES = index.docbook legal.xml

HELP_MEDIA = figures/cpufreq-100.png \
		figures/cpufreq-25.png \
		figures/cpufreq-50.png \
		figures/cpufreq-75.png \
		figures/cpufreq-applet.png \
		figures/cpufreq-applet-preferences.png \
		figures/cpufreq-applet-preferences-smp.png \
		figures/cpufreq-applet-selector-both.png \
		figures/cpufreq-applet-selector.png

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

-include $(top_srcdir)/git.mk
