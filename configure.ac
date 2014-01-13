#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nextwall], [0.3.0], [https://github.com/figure002/nextwall/issues])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/nextwall.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB
AM_PROG_AR

# Check modules.
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([GIO], [gio-2.0])
PKG_CHECK_MODULES([IMAGEMAGICK], [MagickWand])

# Only the developer needs to have Check installed (for unit testing), so
# this dependency is disabled.
#PKG_CHECK_MODULES([CHECK], [check])

# Checks for libraries.
AC_CHECK_LIB([fann], [fann_create_from_file])
AC_CHECK_LIB([m], [floor])
AC_CHECK_LIB([magic], [magic_open])
AC_CHECK_LIB([sqlite3], [sqlite3_exec])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_STRUCT_TIMEZONE

# Checks for library functions.
AC_CHECK_FUNCS([floor localtime_r mkdir realpath sqrt strstr])

# For use of GSettings schemas in Makefiles.
GLIB_GSETTINGS

# GNU help2man creates man pages from --help output; in many cases, this
# is sufficient, and obviates the need to maintain man pages separately.
# However, this means invoking executables, which we generally cannot do
# when cross-compiling, so we test to avoid that (the variable
# "cross_compiling" is set by AC_PROG_CC).
if test $cross_compiling = no; then
  AM_MISSING_PROG(HELP2MAN, help2man)
else
  HELP2MAN=:
fi

AC_CONFIG_FILES([Makefile
				data/Makefile
				gnome-shell-extension/Makefile
				gnome-shell-extension/src/Makefile
				lib/Makefile
				man/Makefile
				src/Makefile
				tests/Makefile
				])
AC_OUTPUT

