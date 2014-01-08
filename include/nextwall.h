/*
  This file is part of nextwall - a wallpaper rotator with some sense of time.

   Copyright 2004, Davyd Madeley <davyd@madeley.id.au>
   Copyright 2010-2013, Serrano Pereira <serrano.pereira@gmail.com>

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

#include <floatfann.h>
#include <sqlite3.h>

/* For tm_gmtoff and tm_zone */
#define _GNU_SOURCE

/* Default size for strings */
#define BUFFER_SIZE 512

/* The maximum number of wallpapers in the wallpaper list */
#define LIST_MAX 2000

/* The nextwall database version */
#define NEXTWALL_DB_VERSION 0.4

/* Wrapper for fprintf() for verbose messages */
#define eprintf(format, ...) do { \
    if (verbose) \
        fprintf(stderr, format, ##__VA_ARGS__); \
} while(0)

/* Default wallpaper directory */
#define DEFAULT_WALLPAPER_DIR "/usr/share/backgrounds/"

/* Function prototypes */
int make_db(sqlite3 *db);
int scan_dir(sqlite3 *db, const char *base, struct fann *ann, int recursive);
int nextwall(sqlite3 *db, const char *path, int brightness);
int get_local_brightness(double lat, double lon);
int remove_wallpaper(sqlite3 *db, char *path);

/* Declare global variables */
extern int verbose;
extern char wallpaper_path[PATH_MAX];

#endif

