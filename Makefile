IDIR = include
CC = gcc
CFLAGS = -Wall -I$(IDIR)

vpath %.c src
vpath %.h include

nextwall: sunriset.o -lm -lsqlite3
