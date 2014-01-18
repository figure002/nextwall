## Makefile.am -- Process this file with automake to produce Makefile.in

TESTS = check-nextwall
check_PROGRAMS = check-nextwall

check_nextwall_SOURCES = check-nextwall.c

check_nextwall_CFLAGS = @CHECK_CFLAGS@

check_nextwall_CPPFLAGS = -I$(top_srcdir)/lib $(GLIB_CFLAGS)

check_nextwall_LDADD = @CHECK_LIBS@ $(top_builddir)/lib/lib$(PACKAGE).a $(GLIB_LIBS)

