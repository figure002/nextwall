## Makefile.am -- Process this file with automake to produce Makefile.in

AM_CFLAGS = -Wall -Werror -I$(top_srcdir)/lib $(GIO_CFLAGS) $(IMAGEMAGICK_CFLAGS)

bin_PROGRAMS = nextwall nextwall-trainer

nextwall_SOURCES = nextwall.c functions.c nextwall.h options.c options.h

nextwall_LDADD = $(top_builddir)/lib/lib$(PACKAGE).a
nextwall_LDADD += -lm -lsqlite3 -lmagic -lfann $(GIO_LIBS) $(IMAGEMAGICK_LIBS)

nextwall_trainer_SOURCES = nextwall-trainer.c trainer-options.c trainer-options.h

nextwall_trainer_LDADD = $(top_builddir)/lib/lib$(PACKAGE).a
nextwall_trainer_LDADD += -lm -lmagic -lfann $(GIO_LIBS) $(IMAGEMAGICK_LIBS)

