#!/usr/bin/bash

# Use grouped output messages
infobegin() {
	echo "::group::${1}"
}
infoend() {
	echo "::endgroup::"
}

# Required packages on Fedora
requires=(
	ccache # Use ccache to speed up build
)

# https://src.fedoraproject.org/cgit/rpms/mate-applets.git
requires+=(
	autoconf-archive
	dbus-glib-devel
	gcc
	git
	gtksourceview4-devel
	gucharmap-devel
	json-glib-devel
	kernel-tools-libs-devel
	libgtop2-devel
	libnl3-devel
	libnotify-devel
	libmateweather-devel
	libsoup3-devel
	libwnck3-devel
	libxml2-devel
	libICE-devel
	libSM-devel
	make
	mate-common
	mate-desktop-devel
	mate-menus-devel
	mate-settings-daemon-devel
	mate-notification-daemon
	mate-panel-devel
	polkit-devel
	redhat-rpm-config
	startup-notification-devel
	upower-devel
	iso-codes-devel
	gobject-introspection-devel
)

infobegin "Update system"
dnf update -y
infoend

infobegin "Install dependency packages"
dnf install -y ${requires[@]}
infoend
