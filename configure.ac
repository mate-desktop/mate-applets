dnl ***************************************************************************
dnl *** configure.in for MATE-APPLETS                                      ***
dnl ***************************************************************************
AC_INIT([mate-applets], [1.27.1], [https://github.com/mate-desktop/mate-applets/issues],
        [mate-applets], [https://mate-desktop.org])
AC_PREREQ(2.59)

AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([1.9 tar-ustar dist-xz no-dist-gzip check-news])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_MACRO_DIR(m4)

dnl ***************************************************************************
dnl *** Minimum library versions for MATE-APPLETS                          ***
dnl ***************************************************************************
GLIB_REQUIRED=2.50.0
GIO_REQUIRED=2.50.0
GTK_REQUIRED=3.22.0
LIBPANEL4_REQUIRED=1.25.2
LIBGTOP_REQUIRED=2.12.0
LIBNOTIFY_REQUIRED=0.7.0
UPOWER_REQUIRED=0.99.8
DBUS_GLIB_REQUIRED=0.74
LIBXML_REQUIRED=2.5.0
POLKIT_REQUIRED=0.97
LIBWNCK_REQUIRED=3.0.0
GUCHARMAP_REQUIRED=3.0.0
dnl ***************************************************************************

AM_MAINTAINER_MODE

PKG_PROG_PKG_CONFIG([0.19])

AC_PROG_CC
AC_STDC_HEADERS
AM_PROG_LIBTOOL
AC_PATH_XTRA

X_LIBS="$X_LIBS $X_PRE_LIBS -lX11 $X_EXTRA_LIBS"
AC_SUBST(X_LIBS)

MATE_COMPILE_WARNINGS

dnl Check for non-portable headers
AC_CHECK_HEADERS([pty.h values.h])

dnl ***************************************************************************
dnl *** Use pkg-config to check for dependencies                            ***
dnl ***************************************************************************

dnl -- check for gio (required) ------------------------------------------
PKG_CHECK_MODULES(GIO, gio-2.0 >= $GIO_REQUIRED gio-unix-2.0)
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

dnl -- Check for GTK+ 3.0 (required) ------------------------------------------
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= $GTK_REQUIRED)
AC_SUBST(GTK_FLAGS)
AC_SUBST(GTK_LIBS)

dnl -- check for libmatepanelapplet4 (required) ------------------------------------
PKG_CHECK_MODULES(MATE_APPLETS4, libmatepanelapplet-4.0 >= $LIBPANEL4_REQUIRED)
AC_SUBST(MATE_APPLETS4_CFLAGS)
AC_SUBST(MATE_APPLETS4_LIBS)

dnl -- check for libgtop (optional) -------------------------------------------
build_gtop_applets=false
PKG_CHECK_MODULES(GTOP_APPLETS,
                 [libgtop-2.0 >= $LIBGTOP_REQUIRED],
                 build_gtop_applets=true,
		 AC_MSG_WARN([libgtop not found. Not building gtop applets.]))
AC_SUBST(GTOP_APPLETS_CFLAGS)
AC_SUBST(GTOP_APPLETS_LIBS)
AM_CONDITIONAL(BUILD_GTOP_APPLETS, $build_gtop_applets)

dnl -- check for libwnck (required) -------------------------------------------
PKG_CHECK_MODULES(LIBWNCK, libwnck-3.0 >= $LIBWNCK_REQUIRED,,
	AC_MSG_ERROR([libwnck is required to build mate-applets.], 1))

AC_SUBST(LIBWNCK_CFLAGS)
AC_SUBST(LIBWNCK_LIBS)

dnl -- check for libnotify (optional) -----------------------------------------
LIBNOTIFY_CFLAGS=
LIBNOTIFY_LIBS=
PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= $LIBNOTIFY_REQUIRED,
		  HAVE_LIBNOTIFY="yes", HAVE_LIBNOTIFY="no")

if test "x$HAVE_LIBNOTIFY" = "xyes"; then
	AC_DEFINE(HAVE_LIBNOTIFY, 1, [libnotify available])
fi

AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)

dnl -- Check for D-Bus -----------------------------------------
AC_ARG_WITH(dbus-sys,
        [AS_HELP_STRING([--with-dbus-sys=<dir>],
        [where D-BUS system.d directory is])])
if ! test -z "$with_dbus_sys" ; then
        DBUS_SYS_DIR="$with_dbus_sys"
else
        DBUS_SYS_DIR='${datadir}/dbus-1/system.d'
fi
AC_SUBST(DBUS_SYS_DIR)

dnl -- check for glib/gobject (required) --------------------------------------
PKG_CHECK_MODULES(CPUFREQ_SELECTOR, glib-2.0 >= $GLIB_REQUIRED gobject-2.0 >= $GLIB_REQUIRED)
AC_SUBST(CPUFREQ_SELECTOR_CFLAGS)
AC_SUBST(CPUFREQ_SELECTOR_LIBS)

dnl -- check for dbus-glib (optional) -----------------------------------------
PKG_CHECK_MODULES(DBUS, dbus-glib-1 >= $DBUS_GLIB_REQUIRED, HAVE_DBUS=yes, HAVE_DBUS=no)
AM_CONDITIONAL(HAVE_DBUS, test "x$HAVE_DBUS" = "xyes")
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

dnl -- check for PolicyKit and PolicyKit-mate (optional) -----------------------------------------
POLKIT_CFLAGS=
POLKIT_LIBS=
AC_ARG_ENABLE([polkit],
    AS_HELP_STRING([--enable-polkit], [Enable PolicyKit support @<:@default yes@:>@]),
    enable_polkit=$enableval,
    enable_polkit=yes)
if test "x$enable_polkit" != "xno"; then
    if test "x$enable_polkit" = "xyes" -a "x$HAVE_DBUS" = "xno"; then
        AC_MSG_ERROR([PolicyKit support requested but dbus-glib-1 not found])
    fi
    PKG_CHECK_MODULES(POLKIT, polkit-gobject-1 >= $POLKIT_REQUIRED, HAVE_POLKIT=yes, HAVE_POLKIT=no)
    if test "x$enable_polkit" = "xyes" -a "x$HAVE_POLKIT" = "xno"; then
        AC_MSG_ERROR([PolicyKit support requested but dependencies not found])
    fi
fi
dnl - This is only so the printout at the end of configure is correct -
if test "x$enable_polkit" = "xno"; then
   HAVE_POLKIT=no
fi
if test "x$HAVE_POLKIT" = "xyes"; then
    AC_DEFINE(HAVE_POLKIT, [1], [PolicyKit available])
fi
AM_CONDITIONAL(HAVE_POLKIT, test "x$HAVE_POLKIT" = "xyes")
AC_SUBST(POLKIT_CFLAGS)
AC_SUBST(POLKIT_LIBS)

dnl -- check for libupower-glib (optional) ------------------------------------
UPOWER_CFLAGS=
UPOWER_LIBS=
AC_ARG_WITH(upower, [  --without-upower        build without upower support])

if test "x$with_upower" != xno; then
   PKG_CHECK_MODULES(UPOWER, upower-glib >= $UPOWER_REQUIRED,
         HAVE_UPOWER="yes",
         HAVE_UPOWER="no")

   if test "x$HAVE_UPOWER" = "xyes"; then
       AC_DEFINE(HAVE_UPOWER, 1, [UPOWER available])
   fi
else
   HAVE_UPOWER="no"
   AC_MSG_WARN(["upower support disabled"])
fi

AC_SUBST(UPOWER_CFLAGS)
AC_SUBST(UPOWER_LIBS)

dnl -- check for gucharmap (optional) -----------------------------------------

PKG_CHECK_EXISTS([gucharmap-2.90 >= $GUCHARMAP_REQUIRED],
                 [have_gucharmap=yes], [have_gucharmap=no])
if test "$have_gucharmap" = "yes"; then
	PKG_CHECK_MODULES([GUCHARMAP], [gucharmap-2.90 >= $GUCHARMAP_REQUIRED])
	AC_DEFINE([HAVE_GUCHARMAP], [1] ,[Gucharmap Available])
else
  AC_MSG_WARN([*** 'charpick' applet will be built without gucharmap support ***])
fi
AC_SUBST(GUCHARMAP_CFLAGS)
AC_SUBST(GUCHARMAP_LIBS)

dnl -- check for libxml2 (required) ------------------------------------------
PKG_CHECK_MODULES(XML2, libxml-2.0 >= $LIBXML_REQUIRED)
AC_SUBST(XML2_CFLAGS)
AC_SUBST(XML2_LIBS)

 dnl -- check for libmateweather (required for mateweather applet) ------------------
build_libmateweather_applets=false
PKG_CHECK_MODULES(LIBMATEWEATHER, mateweather >= 1.17.0,
	          build_libmateweather_applets=true,
		  AC_MSG_WARN([libmateweather not found. Not building the weather applet.]))
AC_SUBST(LIBMATEWEATHER_CFLAGS)
AC_SUBST(LIBMATEWEATHER_LIBS)
AM_CONDITIONAL(BUILD_LIBMATEWEATHER_APPLETS, $build_libmateweather_applets)

dnl ***************************************************************************
dnl *** GSettings                                                           ***
dnl ***************************************************************************
GLIB_GSETTINGS

dnl ***************************************************************************
dnl *** GLIB_COMPILE_RESOURCES                                              ***
dnl ***************************************************************************
AC_ARG_VAR([GLIB_COMPILE_RESOURCES],[glib-compile-resources bin])
AC_PATH_PROG([GLIB_COMPILE_RESOURCES],[glib-compile-resources],[])
if test -z "$GLIB_COMPILE_RESOURCES"; then
  AC_MSG_ERROR([glib-compile-resources not found])
fi

dnl ***************************************************************************
dnl *** XMLLINT                                                             ***
dnl ***************************************************************************
AC_ARG_VAR([XMLLINT],[xmllint bin])
AC_PATH_PROG([XMLLINT],[xmllint],[])
if test -z "$XMLLINT"; then
  AC_MSG_ERROR([xmllint not found])
fi

dnl ***************************************************************************
dnl *** Set up gettext                                                      ***
dnl ***************************************************************************

AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE=mate-applets
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])
AC_DEFINE_DIR(MATELOCALEDIR, "${datadir}/locale", [locale directory])

dnl ***************************************************************************
dnl *** yelp-tools                                                          ***
dnl ***************************************************************************
YELP_HELP_INIT

dnl ***************************************************************************
dnl *** battstat specific checks                                            ***
dnl ***************************************************************************

dnl --disable-battstat=(yes|no)
AC_ARG_ENABLE(battstat,
	      [  --disable-battstat=[@<:@no/yes@:>@]
                          dont build battery monitor applet [@<:@default: no@:>@]],,
	      disable_battstat=no)

HAVE_LIBAPM=no
NEED_LIBAPM=no
ACPIINC=
build_battstat_applet=no

if test x$disable_battstat = xno; then
   build_battstat_applet=yes
   AC_CHECK_HEADERS([err.h sys/sysctl.h])
   case "${host}" in
       # list of supported OS cores that use libapm
       *-*-linux*)
	   NEED_LIBAPM=yes
	   AC_CHECK_LIB(apm,apm_read,[HAVE_LIBAPM=yes],[HAVE_LIBAPM=no])
	   ;;
       # list of supported OS cores that do not use libapm
       i386-*-freebsd*|*-*-netbsd*|*-*-openbsd*|*-*kfreebsd*-gnu)
          if test -n "${OS_SYS}"; then
	       ACPIINC="-I${OS_SYS}"
	   else
	       ACPIINC="-I/usr/src/sys"
	   fi

	   savecppflags=$CPPFLAGS
	   CPPFLAGS="$CPPFLAGS $ACPIINC"
	   AC_CHECK_HEADER(dev/acpica/acpiio.h, have_acpiio=true, have_acpiio=false)
	   if test $have_acpiio = true; then
	     AC_DEFINE(HAVE_ACPIIO, 1, [Define if acpiio.h exists on the system])
	   fi

	   ;;
       *)
	   echo "warning: ${host} is not supported by battstat_applet, not building" >&2
	   build_battstat_applet=no
	   ;;
    esac
fi

AC_SUBST(HAVE_LIBAPM)
AC_SUBST(NEED_LIBAPM)
AC_SUBST(ACPIINC)
AM_CONDITIONAL(HAVE_LIBAPM, test "x$HAVE_LIBAPM" = "xyes")
AM_CONDITIONAL(NEED_LIBAPM, test "x$NEED_LIBAPM" = "xyes")
AM_CONDITIONAL(BUILD_BATTSTAT_APPLET, test x$build_battstat_applet = xyes)

dnl ***************************************************************************
dnl *** Stickynotes specific checks                                         ***
dnl ***************************************************************************
AC_ARG_ENABLE([stickynotes],
    AS_HELP_STRING([--enable-stickynotes], [Enable stickynotes applet.]),
    enable_stickynotes=$enableval,
    enable_stickynotes=yes)
if test "x$enable_stickynotes" = "xyes"; then
    PKG_CHECK_MODULES(STICKYNOTES, gtksourceview-4,
                      have_gtksourceview=yes, have_gtksourceview=no)

    if test "x$enable_stickynotes" = "xyes" -a "x$have_gtksourceview" = "xno"; then
        AC_MSG_ERROR([Stickynotes explicitly requested but gtksourceview not found])
    fi
fi
AM_CONDITIONAL(BUILD_STICKYNOTES_APPLET, test "x$have_gtksourceview" = "xyes")

dnl ***************************************************************************
dnl *** keyboard accessibility status applet check                          ***
dnl ***************************************************************************

savecppflags=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
AC_CHECK_HEADER(X11/extensions/XKB.h, HAVE_XKB=true, HAVE_XKB=false)
AC_SUBST(HAVE_XKB)

AM_CONDITIONAL(APPLET_ACCESSX, $HAVE_XKB)
if test "x$APPLET_ACCESSX" = "xtrue"; then
  AC_MSG_WARN([*** keyboard accessibility status applet will not be built ***])
fi
CPPFLAGS=$savecppflags

dnl ***************************************************************************
dnl *** cpufreq applet check                                                ***
dnl ***************************************************************************

dnl --disable-cpufreq=(yes|no)
AC_ARG_ENABLE(cpufreq,
	      [  --disable-cpufreq[@<:@=no/yes@:>@]
                          dont build cpu frequency scaling monitor applet
			  [@<:@default: no@:>@]],,
	      disable_cpufreq=no)

dnl --enable-frequency-selector=(yes/no)
AC_ARG_ENABLE(frequency-selector,
	      [  --enable-frequency-selector[@<:@=yes/no@:>@]
	                  build frequency selector [@<:@default: yes@:>@]], enable_selector=$enableval,
	      enable_selector=yes)

dnl --enable-suid=(yes/no)
AC_ARG_ENABLE(suid,
	      [  --enable-suid[@<:@=yes/no@:>@]  suid root the frequency selector executable
                          [@<:@default: yes@:>@]],,
	      enable_suid=yes)

if test "x$HAVE_POLKIT" = "xyes"; then
   enable_suid=no
fi

build_cpufreq_applet=yes
AS_IF([test "x$disable_cpufreq" = "xno"], [
  case "${host}" in
    *linux*)
      AC_CHECK_HEADER([cpufreq.h], [
        AC_CHECK_LIB([cpupower], [cpupower_is_cpu_online], [
          AC_DEFINE([HAVE_IS_CPU_ONLINE], 1,
                    [Define to 1 if cpupower_is_cpu_online() is available])
          cpufreq_lib="cpupower"

          AC_CHECK_LIB([cpupower], [cpufreq_get_frequencies], [
            AC_DEFINE([HAVE_GET_FREQUENCIES], 1,
                      [Define to 1 if cpufreq_get_frequencies() is available])
          ])
        ], [
          AC_CHECK_LIB([cpupower], [cpufreq_cpu_exists], [
            cpufreq_lib="cpupower"
          ], [
            AC_CHECK_LIB([cpufreq], [cpufreq_cpu_exists], [
              cpufreq_lib="cpufreq"
            ], [cpufreq_lib=])
          ])
        ])

        AS_IF([test "x$cpufreq_lib" != "x"], [
          LIBCPUFREQ_LIBS="-l$cpufreq_lib"
          AC_DEFINE([HAVE_LIBCPUFREQ], [1], [Have libcpufreq.])
          AC_SUBST([LIBCPUFREQ_LIBS])
        ], [
          AC_MSG_WARN([*** cpufreq applet will not be built ***])
          build_cpufreq_applet=no
        ])
      ])
      ;;
    *)
      AC_MSG_WARN([${host} is not supported by cpufreq applet, not building])
      build_cpufreq_applet=no
      ;;
  esac
], [build_cpufreq_applet=no])

AM_CONDITIONAL(HAVE_LIBCPUFREQ, test x$cpufreq_lib != x)
AM_CONDITIONAL(BUILD_CPUFREQ_APPLET, test x$build_cpufreq_applet = xyes)
AM_CONDITIONAL(BUILD_CPUFREQ_SELECTOR, test x$enable_selector = xyes)
AM_CONDITIONAL(CPUFREQ_SELECTOR_SUID, test x$enable_suid = xyes)

dnl ***************************************************************************
dnl *** timerapplet specific checks                                       ***
dnl ***************************************************************************

AM_CONDITIONAL(BUILD_TIMERAPPLET, test "x$HAVE_LIBNOTIFY" = "xyes")

dnl ***************************************************************************
dnl *** iwlib/netfilter support for netspeed applet                         ***
dnl ***************************************************************************

AC_ARG_WITH(iwevgenie,
            AS_HELP_STRING([--with-iwevgenie],
                           [Build with iwlib support for netspeed applet]),
            [],
            [with_iwevgenie=no])
AC_ARG_WITH(nl,
            AS_HELP_STRING([--with-nl],
                           [Build with netfilter support for netspeed applet]),
            [],
            [with_nl=yes])

if test "x$with_iwevgenie" = "xyes" -a "x$with_nl" = "xyes"; then
  AC_MSG_ERROR([--with-iwevgenie and --with-nl cannot be used at the same time])
fi

have_iwevgenie=no
if test "x$with_iwevgenie" = "xyes"; then
  AC_MSG_CHECKING([for libiw >= 28pre9])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
    #include <iwlib.h>
    #ifndef IWEVGENIE
    #error "not found"
    #endif])],
    [have_iwevgenie=yes],
    AC_MSG_WARN([iwlib.h not found. It's provided by libiw or wireless-tools (depends on the distro)])
  )
  AC_MSG_RESULT([$have_iwevgenie])
fi
if test "x$have_iwevgenie" = "xyes"; then
  AC_DEFINE(HAVE_IW, 1, [Defined if you have libiw/wireless-tools (depends on the distro)])
  IW_LIBS=-liw
  AC_SUBST(IW_LIBS)
fi
AM_CONDITIONAL(HAVE_IW, test "x$have_iwevgenie" = "xyes")

have_nl=no
if test "x$with_nl" = "xyes"; then
  PKG_CHECK_MODULES(NL, libnl-genl-3.0, [have_nl=yes])
fi
if test "x$have_nl" = "xyes"; then
  AC_DEFINE(HAVE_NL, 1, [Enable Generic Netlink Library])
  AC_SUBST(NL_CFLAGS)
  AC_SUBST(NL_LIBS)
fi
AM_CONDITIONAL(HAVE_NL, test "x$have_nl" = "xyes")

dnl ***************************************************************************
dnl *** Check if IPv6 is available                                          ***
dnl ***************************************************************************
AC_MSG_CHECKING([whether to enable ipv6])
AC_ARG_ENABLE(ipv6, [  --enable-ipv6 enable IPv6 extensions], ,enable_ipv6=yes)
if test $enable_ipv6 = yes; then

  dnl Code for checking presence of AF_INET6 on the system.
  AC_TRY_COMPILE([
    #include <sys/types.h>
    #include <sys/socket.h>], [
    socket(AF_INET6, SOCK_STREAM, 0)
    ],
    have_ipv6=yes,
    have_ipv6=no
  )
  AC_MSG_RESULT($have_ipv6)
  if test $have_ipv6 = yes; then
      AC_DEFINE(ENABLE_IPV6, 1, [Define if system is IPv6 enabled])
  fi
fi

dnl ***************************************************************************
dnl *** Check for getaddrinfo                                               ***
dnl ***************************************************************************
have_getaddrinfo=no
AC_CHECK_FUNC(getaddrinfo, have_getaddrinfo=yes)
if test $have_getaddrinfo != yes; then
  # getaddrinfo is not in the default libraries.  See if it's in some other.
  for lib in bsd socket inet; do
    AC_CHECK_LIB($lib, getaddrinfo, [LIBS="$LIBS -l$lib";have_getaddrinfo=yes; break])
  done
fi
if test $have_getaddrinfo = yes; then
  AC_DEFINE(HAVE_GETADDRINFO, 1, [Define if getaddrinfo() exists on the system])
fi

dnl ***************************************************************************
dnl *** Check for presence of tm.tm_gmtoff on the system                    ***
dnl ***************************************************************************
AC_CHECK_MEMBER([struct tm.tm_gmtoff],[struct_tm_tm_gmtoff=true],[struct_tm_tm_gmtoff=false],[#include <time.h>])
case $struct_tm_tm_gmtoff in
        true)
                AC_DEFINE_UNQUOTED(HAVE_TM_TM_GMOFF, 1, Have the tm.tm_gmtoff member.)
                ;;
        *)
                AC_CHECK_DECL(timezone,AC_DEFINE(HAVE_TIMEZONE,1,Have timezone),,
[
#include <time.h>
])
                ;;
esac

dnl ***************************************************************************
dnl *** _NL_MEASUREMENT_MEASUREMENT is an enum and not a define             ***
dnl ***************************************************************************
AC_MSG_CHECKING([for _NL_MEASUREMENT_MEASUREMENT])
AC_TRY_LINK([#include <langinfo.h>], [
char c;
c = *((unsigned char *)  nl_langinfo(_NL_MEASUREMENT_MEASUREMENT));
], gtk_ok=yes, gtk_ok=no)
AC_MSG_RESULT($gtk_ok)
if test "$gtk_ok" = "yes"; then
  AC_DEFINE([HAVE__NL_MEASUREMENT_MEASUREMENT], [1],
            [Define if _NL_MEASUREMENT_MEASUREMENT is available])
fi

AC_ARG_ENABLE([in-process],
              [AS_HELP_STRING([--enable-in-process],
                              [Build all applets in-process])],
              [enable_in_process=$enableval],
              [enable_in_process=no])

# Automake conditional on whether to build in-process
AM_CONDITIONAL([ENABLE_IN_PROCESS], [test "x$enable_in_process" = "xyes"])
# Automake value expected to be substitued in .mate-panel-apple.* for the value of "InProcess"
AS_IF([test "x$enable_in_process" = "xyes"],
      [AC_SUBST([APPLET_IN_PROCESS], [true])],
      [AC_SUBST([APPLET_IN_PROCESS], [false])])
# C conditional on whether to build in-process
AS_IF([test "x$enable_in_process" = "xyes"],
      [AC_DEFINE([ENABLE_IN_PROCESS], [1], [building in-process])])
# Convenience C define selecting the right applet factory
AS_IF([test "x$enable_in_process" = "xyes"],
      [factory=MATE_PANEL_APPLET_IN_PROCESS_FACTORY],
      [factory=MATE_PANEL_APPLET_OUT_PROCESS_FACTORY])
AC_DEFINE_UNQUOTED([PANEL_APPLET_FACTORY], [$factory], [Panel applet factory])

dnl ***************************************************************************
dnl *** Honour aclocal flags                                                ***
dnl ***************************************************************************
ACLOCAL_AMFLAGS="\${ACLOCAL_FLAGS}"
AC_SUBST(ACLOCAL_AMFLAGS)

dnl ***************************************************************************
dnl *** Set install directories                                             ***
dnl ***************************************************************************
iconsdir="${datadir}/icons"
AC_SUBST(iconsdir)
pixmapsdir="${datadir}/pixmaps"
AC_SUBST(pixmapsdir)
pkgdatadir="${datadir}/mate-applets"
AC_SUBST(pkgdatadir)

AC_DEFINE_DIR(MATE_PIXMAPSDIR, "${pixmapsdir}", [Mate Pixmaps Directory])
AC_DEFINE_DIR(PKGDATADIR, "${pkgdatadir}", [Mate Applets Directory])
AC_DEFINE_DIR(DATADIR, "${datadir}", [datadir])
AC_DEFINE_DIR(PKG_DATA_DIR, "${pkgdatadir}", [pkgdatadir])
AC_DEFINE_DIR(SYSCONFDIR, "${sysconfdir}", [sysconfdir])
AC_DEFINE_DIR(LIBDIR, "${libdir}", [libdir])
AC_DEFINE_DIR(PREFIX, "$prefix", [install prefix])

AS_AC_EXPAND(DATADIR, "${datadir}")

AC_SUBST(VERSION)
AC_SUBST(PACKAGE)
AC_SUBST(DATADIR)
AC_SUBST(LIBDIR)
AC_SUBST(PKGDATADIR)

dnl ***************************************************************************
dnl *** Output Makefiles et al                                              ***
dnl ***************************************************************************
AC_CONFIG_FILES([
Makefile
po/Makefile.in
accessx-status/Makefile
accessx-status/data/Makefile
accessx-status/docs/Makefile
accessx-status/icons/Makefile
accessx-status/src/Makefile
drivemount/Makefile
drivemount/data/Makefile
drivemount/data/org.mate.drivemount.gschema.xml
drivemount/help/Makefile
drivemount/src/Makefile
multiload/Makefile
multiload/data/Makefile
multiload/data/org.mate.panel.applet.multiload.gschema.xml
multiload/docs/Makefile
multiload/src/Makefile
charpick/Makefile
charpick/org.mate.panel.applet.charpick.gschema.xml
charpick/help/Makefile
mateweather/Makefile
mateweather/data/Makefile
mateweather/docs/Makefile
mateweather/src/Makefile
geyes/Makefile
geyes/data/Makefile
geyes/data/org.mate.panel.applet.geyes.gschema.xml
geyes/icons/Makefile
geyes/themes/Makefile
geyes/docs/Makefile
geyes/src/Makefile
battstat/Makefile
battstat/org.mate.panel.applet.battstat.gschema.xml
battstat/apmlib/Makefile
battstat/sounds/Makefile
battstat/docs/Makefile
man/Makefile
stickynotes/Makefile
stickynotes/org.mate.stickynotes.gschema.xml
stickynotes/pixmaps/Makefile
stickynotes/docs/Makefile
trashapplet/Makefile
trashapplet/data/Makefile
trashapplet/src/Makefile
trashapplet/docs/Makefile
cpufreq/Makefile
cpufreq/data/Makefile
cpufreq/data/org.mate.panel.applet.cpufreq.gschema.xml
cpufreq/src/Makefile
cpufreq/src/cpufreq-selector/Makefile
cpufreq/pixmaps/Makefile
cpufreq/help/Makefile
timerapplet/Makefile
timerapplet/data/Makefile
timerapplet/data/org.mate.panel.applet.timer.gschema.xml
timerapplet/src/Makefile
command/Makefile
command/data/Makefile
command/data/org.mate.panel.applet.command.gschema.xml
command/src/Makefile
netspeed/Makefile
netspeed/data/Makefile
netspeed/data/org.mate.panel.applet.netspeed.gschema.xml
netspeed/icons/Makefile
netspeed/src/Makefile
netspeed/help/Makefile
])
AC_OUTPUT

dnl ***************************************************************************
dnl *** Display Summary                                                     ***
dnl ***************************************************************************
echo "
Configure summary:

    ${PACKAGE_STRING}
    `echo $PACKAGE_STRING | sed "s/./=/g"`

    Prefix:                        ${prefix}
    Source code location:          ${srcdir}
    Compiler:                      ${CC}
    Compiler flags:                ${CFLAGS}
    Compiler warnings:             ${WARN_CFLAGS}

    Building:
        - accessx-status           $HAVE_XKB
        - battstat                 $build_battstat_applet
        - charpick                 always
            - gucharmap support    $have_gucharmap
        - cpufreq                  $build_cpufreq_applet
            - building selector    $enable_selector
            - using PolicyKit      $HAVE_POLKIT
            - enabling suid bit    $enable_suid
        - drivemount               always
        - geyes                    always
        - mateweather              $build_libmateweather_applets
        - multiload                $build_gtop_applets
        - netspeed                 $build_gtop_applets
            - iwlib support        $have_iwevgenie
            - netfilter support    $have_nl
        - stickynotes              $enable_stickynotes
        - timerapplet              $HAVE_LIBNOTIFY
        - trashapplet              always

    Using DBUS:                    $HAVE_DBUS
    Using UPOWER:                  $HAVE_UPOWER
    Using libnotify:               $HAVE_LIBNOTIFY
    Enabling IPv6:                 $have_ipv6
    Build in-process:              $enable_in_process
" >&2
