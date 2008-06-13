#                                               -*- Autoconf -*-
# Process this file with autoreconf to produce a configure script.

m4_include(as_ac_expand.m4)

AC_PREREQ(2.61)
dnl Each time you want an updated version number for your build, you need to
dnl (re-)run autoreconf.
AC_INIT([awesome window manager],
        [m4_esyscmd([./build-utils/package-version . version-stamp])],
        [http://awesome.naquadah.org/bugs/],
        [awesome])
AC_CONFIG_AUX_DIR([auto-aux])
AC_CONFIG_SRCDIR([awesome.c])
dnl The AC_CONFIG_HEADER and awesome's own config.h are different beasts
dnl AC_CONFIG_HEADER([config.h])
dnl Other options:
dnl    check-news   to check that NEWS has been adapted to current version
AM_INIT_AUTOMAKE([
-Wall -Werror
1.10
dist-bzip2
filename-length-max=99
foreign
std-options
subdir-objects
])
AC_CONFIG_HEADER([config.h])

AC_DEFINE_UNQUOTED([AWESOME_RELEASE], [m4_esyscmd([./build-utils/package-codename . codename-stamp])], [release code name])

AC_MSG_CHECKING([package version according to autoconf])
AC_MSG_RESULT([${PACKAGE_VERSION}])
AC_MSG_CHECKING([package version according to version script])
version_script=`"${srcdir}/build-utils/package-version" "${srcdir}" version-stamp`
AC_MSG_RESULT([${version_script}])

# Checks for programs.
AC_PROG_SED
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

AC_ARG_VAR([ASCIIDOC], [asciidoc doc generation tool])
AC_ARG_VAR([XMLTO], [xmlto converter from xml to other formats])
AC_ARG_VAR([DOXYGEN], [doxygen source documentation system])

test "x$ASCIIDOC" = "x"  && AC_PATH_PROG([ASCIIDOC], [asciidoc], [:])
test "x$XMLTO"    = "x"  && AC_PATH_PROG([XMLTO], [xmlto], [:])
test "x$DOXYGEN"  = "x"  && AC_PATH_PROG([DOXYGEN], [doxygen], [:])

test "x$ASCIIDOC" = "x:" && AC_MSG_WARN([awesome man page generation requires asciidoc])
test "x$XMLTO"    = "x:" && AC_MSG_WARN([awesome man page generation requires xmlto])
test "x$DOXYGEN"  = "x:" && AC_MSG_WARN([awesome developer doc generation requires doxygen])

AM_CONDITIONAL([HAVE_ASCIIDOC], [test "x$ASCIIDOC" != "x:"])
AM_CONDITIONAL([HAVE_XMLTO],    [test "x$XMLTO"    != "x:"])
AM_CONDITIONAL([HAVE_DOXYGEN],  [test "x$DOXYGEN"  != "x:"])

xmlto_man_works=no
if test ="x$XMLTO" != "x:"; then dnl try generating an example man page
	rm -f conf-example.1
	cat>conf-example.1.xml<<MANEOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE refentry PUBLIC "-//OASIS//DTD DocBook XML V4.2//EN"
          "http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd">
<refentry>
<refmeta>
<refentrytitle>conf-example</refentrytitle>
<manvolnum>1</manvolnum>
</refmeta>
<refnamediv>
<refname>conf-example</refname>
<refpurpose>'configure' time test for working 'xmlto man'</refpurpose>
</refnamediv>
<refsynopsisdiv>
<simpara>foo</simpara>
</refsynopsisdiv>
<refsect1>
<title>DESCRIPTION</title>
<simpara>bar</simpara>
</refsect1>
</refentry>
MANEOF
	AC_MSG_CHECKING([whether xmlto can create a man page])
	if ${XMLTO} man conf-example.1.xml >/dev/null 2>&1 && test -s conf-example.1; then
		xmlto_man_works=yes
		AC_MSG_RESULT([yes])
		rm -f conf-example.1 conf-example.1.xml
	else
		AC_MSG_RESULT([no])
		AC_MSG_WARN([awesome man page generation requires 'xmlto man', but it does not work.
Do you need to (properly) install http://docbook.sourceforge.net/projects/xsl/?])
	fi
fi
AM_CONDITIONAL([XMLTO_MAN_WORKS], [test "x$xmlto_man_works" = "xyes"])

AM_CONDITIONAL([USING_GCC], [test "x$GCC" = "xyes"])

AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED([AWESOME_COMPILE_MACHINE], ["$host"], [build for this machine])

AC_DEFINE_UNQUOTED([AWESOME_COMPILE_HOSTNAME], ["${ac_hostname}"], [build host])

AC_ARG_VAR([WHOAMI], [whoami(1) command])
AC_PATH_PROG([WHOAMI], [whoami], [false])
AS_IF([test "x$WHOAMI" = "xfalse"], [aw_whoami="???"], [aw_whoami=`$WHOAMI`])
AC_DEFINE_UNQUOTED([AWESOME_COMPILE_BY], ["$aw_whoami"], [build user])

AC_ARG_VAR([LUA], [Lua interpreter])
AC_PATH_PROG([LUA], [lua], [:])
test "x$LUA" = "x" && AC_MSG_WARN([awesome compilation requires lua])

AC_ARG_VAR([LUAC], [Lua compiler])
AC_PATH_PROG([LUAC], [luac], [:])
test "x$LUAC" = "x" && AC_MSG_WARN([awesome compilation requires luac])

AS_AC_EXPAND(awesomelibdir, "$datarootdir/awesome/lib")
AC_DEFINE_UNQUOTED([AWESOME_LUA_LIB_PATH], "${awesomelibdir}", [awesome lib path])
AS_AC_EXPAND(awesomesysconfdir, "$sysconfdir/awesome")
AC_DEFINE_UNQUOTED([AWESOME_CONF_PATH], "${awesomesysconfdir}", [awesome conf path])

# Checks for libraries.
AC_PATH_XTRA
PKG_CHECK_MODULES([pangocairo], [pangocairo],,
   [AC_MSG_ERROR([awesome requires pangocairo.])])
PKG_CHECK_MODULES([Lua], [lua >= 5.1],,
   [PKG_CHECK_MODULES([Lua], [lua5.1],,
                      [AC_MSG_ERROR([awesome requires Lua >= 5.1.])])])
PKG_CHECK_MODULES([glib], [glib-2.0],,
   [AC_MSG_ERROR([awesome requires glib-2.0.])])

AC_ARG_WITH([dbus], AS_HELP_STRING([--with-dbus], [Build with D-BUS (default: enabled)]), [], [with_dbus=yes])
if test "x$with_dbus" == "xyes"; then
	PKG_CHECK_MODULES([dbus], [dbus-1],,
		[AC_MSG_ERROR([awesome requires dbus-1.])])
	AC_DEFINE([WITH_DBUS],1,[Defined to use D-BUS])
fi

AC_ARG_WITH([imlib2], AS_HELP_STRING([--with-imlib2], [Build with Imlib2 (default: disabled)]))

if test "x$with_imlib2" == "xyes"; then
    PKG_CHECK_MODULES([imlib2], [imlib2],,
        [AC_MSG_ERROR([Cannot fined Imlib2.])])
    AC_DEFINE([WITH_IMLIB2],1,[Defined to use Imlib2])
else
    PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.2],,
        [AC_MSG_ERROR([Cannot find gtk+-2.0 >= 2.2])])
fi
PKG_CHECK_MODULES([xcb], [xcb],,
   [AC_MSG_ERROR([awesome requires xcb.])])
PKG_CHECK_MODULES([xcb_event], [xcb-event],,
   [AC_MSG_ERROR([awesome requires xcb-event.])])
PKG_CHECK_MODULES([xcb_randr], [xcb-randr],,
   [AC_MSG_ERROR([awesome requires xcb-randr.])])
PKG_CHECK_MODULES([xcb_xinerama], [xcb-xinerama],,
   [AC_MSG_ERROR([awesome requires xcb-xinerama.])])
PKG_CHECK_MODULES([xcb_shape], [xcb-shape],,
   [AC_MSG_ERROR([awesome requires xcb-shape.])])
PKG_CHECK_MODULES([xcb_aux], [xcb-aux],,
   [AC_MSG_ERROR([awesome requires xcb-aux.])])
PKG_CHECK_MODULES([xcb_atom], [xcb-atom],,
   [AC_MSG_ERROR([awesome requires xcb-atom.])])
PKG_CHECK_MODULES([xcb_keysyms], [xcb-keysyms],,
   [AC_MSG_ERROR([awesome requires xcb-keysyms.])])
PKG_CHECK_MODULES([xcb_render], [xcb-render],,
   [AC_MSG_ERROR([awesome requires xcb-render.])])
PKG_CHECK_MODULES([xcb_icccm], [xcb-icccm],,
   [AC_MSG_ERROR([awesome requires xcb-icccm.])])
PKG_CHECK_MODULES([cairo_xcb], [cairo-xcb],,
   [AC_MSG_ERROR([awesome requires cairo-xcb.])])
AC_CHECK_LIB(readline, readline,
             [AC_SUBST([readline_LIBS], ["-lreadline -lncurses"])],
             [AC_MSG_FAILURE([awesome needs readline])], -lncurses)

# Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h locale.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([getline memchr regcomp select setenv socket strchr strrchr strstr])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([awesome.doxygen])
AC_OUTPUT
