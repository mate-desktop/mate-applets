#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Debian
requires=(
	ccache # Use ccache to speed up build
)

# https://salsa.debian.org/debian-mate-team/mate-applets
requires+=(
	autopoint
	gcc
	git
	libcpupower-dev
	libdbus-1-dev
	libdbus-glib-1-dev
	libglib2.0-dev
	libgtk-3-dev
	libgtksourceview-4-dev
	libgtop2-dev
	libgucharmap-2-90-dev
	libiw-dev
	libmate-desktop-dev
	libmate-menu-dev
	libmate-panel-applet-dev
	libmateweather-dev
	libnl-genl-3-dev
	libnotify-dev
	libpolkit-gobject-1-dev
	libupower-glib-dev
	libwnck-3-dev
	libx11-dev
	libxml2-dev
	make
	mate-common
	x11proto-kb-dev
	yelp-tools
	iso-codes
	gobject-introspection
	libgirepository1.0-dev
)

infobegin "Update system"
apt-get update -qq
infoend

infobegin "Install dependency packages"
env DEBIAN_FRONTEND=noninteractive \
	apt-get install --assume-yes \
	${requires[@]}
infoend
