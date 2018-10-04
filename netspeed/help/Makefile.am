@YELP_HELP_RULES@

HELP_ID = mate-netspeed-applet
HELP_FILES = index.docbook legal.xml
HELP_MEDIA = \
	figures/details.png \
	figures/ethernet.png \
	figures/eth_sum_48.png \
	figures/loopback.png \
	figures/netspeed_applet.png \
	figures/plip.png \
	figures/ppp.png \
	figures/settings.png \
	figures/wavelan.png

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

EXTRA_DIST = INSTRUCTIONS
