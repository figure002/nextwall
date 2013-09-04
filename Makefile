IDIR = include
CC = gcc
CFLAGS = -Wall -I$(IDIR) $(shell pkg-config --cflags gio-2.0 MagickWand)

vpath %.c src
vpath %.h include

nextwall: sunriset.h nextwall.h -lm -lsqlite3 -lmagic $(shell pkg-config --libs gio-2.0 MagickWand)

