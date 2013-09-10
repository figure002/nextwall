#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([nextwall], [0.3.0], [serrano.pereira@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/nextwall.c])
AC_CONFIG_HEADERS([include/config.h])

# Checks for programs.
AC_PROG_CC

# Check modules.
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.22])
PKG_CHECK_MODULES([IMAGEMAGICK], [MagickWand >= 6.6.0])

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lfann':
AC_CHECK_LIB([fann], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lmagic':
AC_CHECK_LIB([magic], [main])
# FIXME: Replace `main' with a function in `-lsqlite3':
AC_CHECK_LIB([sqlite3], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_STRUCT_TIMEZONE

# Checks for library functions.
AC_CHECK_FUNCS([floor localtime_r mkdir realpath sqrt strstr])

AC_CONFIG_FILES([Makefile
				src/Makefile
				data/Makefile])
AC_OUTPUT

