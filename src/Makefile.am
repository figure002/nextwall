## Makefile.am -- Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror -I$(top_srcdir)/include $(GIO_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = nextwall
EXTRA_DIST = $(top_srcdir)/include/*.h $(top_srcdir)/data/ann/nextwall.net \
	$(top_srcdir)/src/util/Makefile $(top_srcdir)/src/util/train_ann.c

man1_MANS = $(top_srcdir)/man/nextwall.1
EXTRA_DIST += $(top_srcdir)/man/*

nextwall_SOURCES = nextwall.c
nextwall_LDADD = -lm -lsqlite3 -lmagic -lfann $(GIO_LIBS) $(IMAGEMAGICK_LIBS)

