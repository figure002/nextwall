## Makefile.am -- Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libnextwall.a

libnextwall_a_SOURCES = database.c database.h std.c std.h gnome.c gnome.h \
	image.c image.h cfgpath.h sunriset.c sunriset.h

AM_CPPFLAGS = -Wall -Werror $(GIO_CFLAGS) $(IMAGEMAGICK_CFLAGS)

