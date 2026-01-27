#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Archlinux
requires=(
	ccache # Use ccache to speed up build
	clang  # Build with clang on Archlinux
)

# https://gitlab.archlinux.org/archlinux/packaging/packages/mate-applets
requires+=(
	autoconf-archive
	cpupower
	dbus-glib
	gcc
	git
	glib2-devel
	gtksourceview4
	gucharmap
	intltool
	itstool
	json-glib
	libgtop
	libnl
	libnotify
	libsoup3
	make
	mate-common
	mate-panel
	polkit
	upower
	which
	wireless_tools
	yelp-tools
)

infobegin "Update system"
pacman --noconfirm -Syu
infoend

infobegin "Install dependency packages"
pacman --noconfirm -S ${requires[@]}
infoend
