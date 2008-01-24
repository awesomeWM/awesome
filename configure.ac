#                                               -*- Autoconf -*-
# Process this file with autoreconf to produce a configure script.

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

AC_DEFINE_UNQUOTED([RELEASE], ["Productivity Breaker"])
AC_DEFINE_UNQUOTED([AWESOME_COMPILE_HOST], ["`hostname`"])
AC_DEFINE_UNQUOTED([AWESOME_COMPILE_BY], ["`whoami`"])

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

test "x$ASCIIDOC" = "x:" && AC_MSG_WARN([awesome requires asciidoc])
test "x$XMLTO"    = "x:" && AC_MSG_WARN([awesome requires xmlto])
test "x$DOXYGEN"  = "x:" && AC_MSG_WARN([awesome requires doxygen])

AM_CONDITIONAL([HAVE_ASCIIDOC], [test "x$ASCIIDOC" != "x:"])
AM_CONDITIONAL([HAVE_XMLTO],    [test "x$XMLTO"    != "x:"])
AM_CONDITIONAL([HAVE_DOXYGEN],  [test "x$DOXYGEN"  != "x:"])

AM_CONDITIONAL([USING_GCC], [test "x$GCC" = "xyes"])

# Checks for libraries.
AC_PATH_XTRA
PKG_CHECK_MODULES([CAIRO], [cairo],,
   [AC_MSG_ERROR([awesome requires cairo.])])
PKG_CHECK_MODULES([CONFUSE], [libconfuse >= 2.6],,
   [AC_MSG_ERROR([awesome requires libconfuse >= 2.6.])])
PKG_CHECK_MODULES([XFT], [xft],,
   [AC_MSG_ERROR([awesome requires xft.])])
PKG_CHECK_MODULES([XINERAMA], [xinerama],,
   [AC_MSG_ERROR([awesome requires Xinerama.])])
PKG_CHECK_MODULES([XRANDR], [xrandr],,
   [AC_MSG_ERROR([awesome requires Xrandr.])])

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
AC_CHECK_FUNCS([memchr regcomp select setenv socket strchr strrchr strstr])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([awesome.doxygen])
AC_OUTPUT
