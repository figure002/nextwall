/*
  This file is part of nextwall - a wallpaper rotator with some sense of time.

   Copyright 2004, Davyd Madeley <davyd@madeley.id.au>
   Copyright 2010-2013, Serrano Pereira <serrano@bitosis.nl>

   Nextwall is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Nextwall is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NEXTWALL_H
#define NEXTWALL_H

#include <gio/gio.h>
#include <sqlite3.h>

/* Default wallpaper directory */
#define DEFAULT_WALLPAPER_DIR "/usr/share/backgrounds/"

/* Wrapper for fprintf() for verbose messages */
#define eprintf(format, ...) do { \
    if (nextwall_verbose) \
        fprintf(stderr, format, ##__VA_ARGS__); \
} while(0)

struct wallpaper_state {
    char *dir;      /* Wallpaper base directory */
    char *current;  /* Current wallpaper */
    char *path;     /* Path for next wallpaper */
};

int get_local_brightness(double lat, double lon);
int set_wallpaper(GSettings *settings,
                  sqlite3 *db,
                  int brightness,
                  struct wallpaper_state *state,
                  int print_only);

/* Set to 1 to display verbose messages */
extern int nextwall_verbose;

#endif
