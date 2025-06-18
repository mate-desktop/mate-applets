SUBDIRS = art

# ******************************************************************************
# Panel stuff
# ******************************************************************************
appletdir       = $(datadir)/mate-panel/applets

# ******************************************************************************
# GSettings schemas
# ******************************************************************************
gschema_in_files = org.mate.panel.applet.invest.gschema.xml.in
gsettings_SCHEMAS = $(gschema_in_files:.xml.in=.xml)
@GSETTINGS_RULES@

# ******************************************************************************
# Misc data
# ******************************************************************************

investbindir = $(libdir)/invest-applet

# ******************************************************************************
# Build rules
# ******************************************************************************

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	*.gschema.valid

EXTRA_DIST = \
	org.mate.applets.InvestApplet.mate-panel-applet.in.in \
	$(service_in_files) \
	$(gschema_in_files)

-include $(top_srcdir)/git.mk
