## Makefile.am -- Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror -I$(top_srcdir)/include $(GIO_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = nextwall nextwall-trainer
EXTRA_DIST = $(top_srcdir)/include/*.h

nextwall_SOURCES = nextwall.c functions.c options.c std.c gnome.c image.c
nextwall_LDADD = -lm -lsqlite3 -lmagic -lfann $(GIO_LIBS) $(IMAGEMAGICK_LIBS)

nextwall_trainer_SOURCES = nextwall-trainer.c trainer-options.c std.c gnome.c image.c
nextwall_trainer_LDADD = -lm -lmagic -lfann $(GIO_LIBS) $(IMAGEMAGICK_LIBS)

