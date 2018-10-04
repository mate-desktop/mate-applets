@YELP_HELP_RULES@

HELP_ID = mate-accessx-status
HELP_FILES = index.docbook legal.xml

HELP_MEDIA = figures/accessx_bounce-keys.png \
	figures/accessx_mouse-keys.png \
	figures/accessx_slow-keys.png \
	figures/accessx-status-applet.png \
	figures/accessx-status-disabled.png \
	figures/accessx_sticky-altGr-key.png \
	figures/accessx_sticky-keys.png \
	figures/accessx_sticky-meta-key.png \
	figures/accessx_sticky-shift-key.png \
	figures/accessx_sticky-windows-key.png

# Add linguas to be ignored, e.g. IGNORE_HELP_LINGUAS = ca de es fr
IGNORE_HELP_LINGUAS =
HELP_LINGUAS = $(if $(IGNORE_HELP_LINGUAS), \
	$(filter-out $(IGNORE_HELP_LINGUAS),$(subst /,,$(dir $(wildcard */*.po)))), \
	$(subst /,,$(dir $(wildcard */*.po))) )

-include $(top_srcdir)/git.mk
