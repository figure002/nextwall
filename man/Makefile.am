## Makefile.am -- Process this file with automake to produce Makefile.in

man_MANS = nextwall.1
EXTRA_DIST = $(man_MANS) nextwall.h2m
MAINTAINERCLEANFILES = $(man_MANS)

# The man pages depend on the --help strings and the version number.
common_mandeps = $(top_srcdir)/configure.ac nextwall.h2m

# Use -o so that the `missing' program can infer the output file.
# Only depend on options.c, because the man file needs to be manually
# updated every time it is generated.
nextwall.1: $(top_srcdir)/src/options.c $(common_mandeps)
	$(HELP2MAN) --no-info -i nextwall.h2m -o $@ ../src/nextwall

