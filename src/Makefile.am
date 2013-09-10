## Makefile.am -- Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror -I../include $(GIO_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = nextwall

nextwall_SOURCES = nextwall.c sunriset.h nextwall.h
nextwall_LDADD = -lm -lsqlite3 -lmagic -lfann $(GIO_LIBS) $(IMAGEMAGICK_LIBS)

