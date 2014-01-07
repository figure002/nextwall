CC = gcc
CFLAGS = -Wall -I../../include $(shell pkg-config --cflags gio-2.0 MagickWand)

train_ann: ../std.c ../image.c ../gnome.c -lm -lmagic -lfann $(shell pkg-config --libs gio-2.0 MagickWand)

.PHONY: clean
clean:
	rm train_ann

