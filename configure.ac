#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nextwall], [0.3.0], [https://github.com/figure002/nextwall/issues])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/nextwall.c])
AC_CONFIG_HEADERS([include/config.h])

# Checks for programs.
AC_PROG_CC

# Check modules.
PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([GIO], [gio-2.0])
PKG_CHECK_MODULES([IMAGEMAGICK], [MagickWand])

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

AC_CONFIG_FILES([Makefile
				src/Makefile
				data/Makefile
				icons/Makefile])
AC_OUTPUT

