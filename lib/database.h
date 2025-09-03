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

#ifndef DATABASE_H
#define DATABASE_H

#include <stdbool.h>
#include <floatfann.h>
#include <sqlite3.h>

/* The nextwall database version */
#define NEXTWALL_DB_VERSION 0.5

/* The maximum number of wallpapers in the wallpaper list */
#define LIST_MAX 2000

int create_database(sqlite3 *db);
int scan_dir(sqlite3 *db, const char *base, struct fann *ann, int recursive);
int nextwall(sqlite3 *db, const char *base, int brightness, char *result_path);
int set_path_from_id(sqlite3 *db, int id, char *result_path);
int remove_wallpaper(sqlite3 *db, char *path, bool trash_file);
int get_terminal_width();

#endif
