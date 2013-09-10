## Makefile.am -- Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror -I$(top_srcdir)/include $(GIO_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = nextwall
EXTRA_DIST = $(top_srcdir)/include/*.h $(top_srcdir)/data/ann/nextwall.net

nextwall_SOURCES = nextwall.c
nextwall_LDADD = -lm -lsqlite3 -lmagic -lfann $(GIO_LIBS) $(IMAGEMAGICK_LIBS)

