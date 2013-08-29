IDIR = include
CC = gcc
CFLAGS = -Wall -I$(IDIR)

vpath %.c src
vpath %.h include

nextwall: sunriset.o nextwall.h -lm -lsqlite3
