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

#define _GNU_SOURCE     /* asprintf */

#include <errno.h>      /* errno */
#include <stdio.h>      /* perror */
#include <floatfann.h>
#include <magic.h>
#include <limits.h>     /* realpath */
#include <stdlib.h>     /* realpath */
#include <string.h>     /* strcmp */
#include <sqlite3.h>
#include <sys/types.h>  /* open opendir */
#include <sys/stat.h>   /* open opendir */
#include <dirent.h>     /* open opendir */
#include <fcntl.h>      /* open opendir */
#include <bsd/string.h> /* strlcpy strlcat */

#include "database.h"
#include "gnome.h"      /* file_trash */
#include "image.h"      /* get_image_info */
#include "std.h"        /* get_brightness */

extern int errno;

static int wallpaper_list_populated = 0;
static int wallpaper_count = 0;
static int wallpaper_current = 0;
static int wallpaper_list[LIST_MAX];

/* Function prototypes */
static int save_image_info(sqlite3_stmt *stmt, struct fann *ann, const char *path);
static int is_known_image(sqlite3 *db, const char *path);
static int callback_known_image(void *param, int argc, char **argv, char **colnames);
static int callback_wallpaper_list(void *param, int argc, char **argv, char **colnames);
static int callback_wallpaper_path(void *param, int argc, char **argv, char **colnames);

/**
  Create a new nextwall database.

  @param[in] db The database handler.
  @return Returns 0 on success, -1 on failure.
 */
int create_database(sqlite3 *db) {
    int rc = 0, rc2 = 0;
    char *query, *mquery = NULL;

    query = "CREATE TABLE wallpapers (" \
        "id INTEGER PRIMARY KEY," \
        "path TEXT," \
        "kurtosis FLOAT," \
        "lightness FLOAT," \
        "brightness INTEGER" \
        ");";

    rc = sqlite3_exec(db, query, NULL, NULL, NULL);

    if (rc != SQLITE_OK)
        goto Return;

    query = "CREATE TABLE info (" \
        "id INTEGER PRIMARY KEY," \
        "name VARCHAR," \
        "value VARCHAR);";

    rc = sqlite3_exec(db, query, NULL, NULL, NULL);

    if (rc != SQLITE_OK)
        goto Return;

    if ((rc2 = asprintf(&mquery,
            "INSERT INTO info VALUES (null, 'version', %f);",
            NEXTWALL_DB_VERSION)) == -1) {
        fprintf(stderr, "asprintf() failed: %s\n", strerror(errno));
        goto Return;
    }

    rc = sqlite3_exec(db, mquery, NULL, NULL, NULL);

    if (rc != SQLITE_OK)
        goto Return;

    rc = sqlite3_exec(db,
        "CREATE UNIQUE INDEX wallpapers_path_idx ON wallpapers (path);",
        NULL, NULL, NULL);

    goto Return;

Return:
    if (mquery)
        free(mquery);

    if (rc != SQLITE_OK || rc2 == -1)
        return -1;

    return 0;
}

/**
  Scan the directory for new wallpapers.

  The path of each image file that is found in the directory is saved along
  with additional information (e.g. kurtosis, lightness) to the database. It
  will use the Artificial Neural Network to define the brightness value of each
  image.

  @param[in] db The database handler.
  @param[in] name The base directory.
  @param[in] recursive If set to 1, the base directory is scanned recursively.
  @return The number of new wallpapers that were found.
 */
int scan_dir(sqlite3 *db, const char *base, struct fann *ann, int recursive) {
    int found = 0;
    char *path_tmp = NULL;
    char *query;
    char path[PATH_MAX];
    const char *tail = 0;
    struct dirent *entry;
    DIR *dir;
    sqlite3_stmt *stmt;

    // Initialize Magic Number Recognition Library
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic, NULL);

    if (recursive < 2) {
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    }

    if (!(dir = opendir(base))) {
        return found;
    }

    if (!(entry = readdir(dir))) {
        return found;
    }

    // Prepare INSERT statement
    query = "INSERT INTO wallpapers VALUES (null, @PATH, @KUR, @LGT, @BRI);";
    sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, &tail);

    do {
        if (asprintf(&path_tmp, "%s/%s", base, entry->d_name) == -1) {
            perror("asprintf");
            goto Return;
        }

        if (realpath(path_tmp, path) == NULL) {
            perror("realpath");
            goto Return;
        }

        if (entry->d_type == DT_DIR) {
            if (!recursive) {
                continue;
            }

            if (strcmp(entry->d_name, ".") == 0 || \
                strcmp(entry->d_name, "..") == 0 || \
                strcmp(entry->d_name, ".thumbs") == 0) {
                continue;
            }

            fprintf(stderr, "\nEntering %s...", path);
            found += scan_dir(db, path, ann, recursive + 1);
        }
        else if (strstr(magic_file(magic, path), "image")) {
            if (is_known_image(db, path)) {
                continue;
            }

            if (save_image_info(stmt, ann, path) == 0) {
                ++found;
                fprintf(stderr, ".");
            }
            else {
                fprintf(stderr, "\nError: Failed to save image info for %s\n", path);
                goto Return;
            }
        }
        else if (entry->d_type == DT_UNKNOWN) {
            fprintf(stderr, "\nCould not determine file type of '%s'; skipping...\n", path);
        }
    } while ((entry = readdir(dir)));

    goto Return;

Return:
    if (recursive < 2) {
        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
        sqlite3_finalize(stmt);
    }

    magic_close(magic);

    if (path_tmp) {
        free(path_tmp);
    }

    closedir(dir);

    return found;
}

/**
  Check if a wallpaper is already present in the nextwall database.

  @param[in] db The database handler.
  @param[in] path The absolute path of the wallpaper to check.
  @return Returns 1 if the wallpaper is known, 0 otherwise.
 */
int is_known_image(sqlite3 *db, const char *path) {
    int known = 0;
	int rc = 0;
    char *query = NULL;

    if (asprintf(&query, "SELECT id FROM wallpapers WHERE path='%s';", path) == -1) {
        fprintf(stderr, "asprintf() failed: %s\n", strerror(errno));
        goto on_error;
    }

    rc = sqlite3_exec(db, query, callback_known_image, &known, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", query);
        goto on_error;
    }

    free(query);

    return known;

on_error:
    free(query);
    exit(1);
}

/**
  Callback function for is_known_image().

  Sets the first argument to the callback to 1.

  @param[out] param Pointer to an integer that will be set to 1.
  @param[in] argc The number of columns in the result.
  @param[in] argv An array of pointers to strings obtained for each column.
  @param[in] colnames An array of pointers to strings where each entry
             represents the name of corresponding result column.
 */
int callback_known_image(void *param, int argc, char **argv, char **colnames) {
    int *known = (int *)param;
    *known = 1;

    return 0;
}

/**
  Saves the wallpaper information to the nextwall database.

  Saves the wallpaper path along with the kurtosis, lightness, and brightness
  value for the wallpaper.

  @param[in] The prepared insert statement.
  @param[in] The Artificial Neural Network.
  @param[in] The absolute path of the wallpaper file.
  @return Returns 0 on success, -1 otherwise.
 */
int save_image_info(sqlite3_stmt *stmt, struct fann *ann, const char *path) {
    double kurtosis, lightness;
    int brightness;
	int rc = 0;

    // Get the lightness for this image
    if (get_image_info(path, &kurtosis, &lightness) == -1) {
        return -1;
    }

    // Get image brigthness
    brightness = get_brightness(ann, kurtosis, lightness);

    // Bind values to prepared statement
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, kurtosis);
    sqlite3_bind_double(stmt, 3, lightness);
    sqlite3_bind_int(stmt, 4, brightness);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        return -1;
    }

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);

    return 0;
}

/**
  Select a random wallpaper from the nextwall database.

  @param[in] db The database handler.
  @param[in] base The base directory from which to select wallpapers.
  @param[in] brightness If set to 0, 1, or 2, wallpapers matching this
             brightness value are returned.
  @param[out] path Will be set to the path of the randomly selected wallpaper.
  @return Returns the ID of a randomly selected wallpaper on success, -1
          otherwise.
 */
int nextwall(sqlite3 *db, const char *base, int brightness, char *path) {
    int id;
    int rc = 0;
    char *query = NULL;
    char *query2 = NULL;

    if (!wallpaper_list_populated) {
        char real_base[PATH_MAX];

        /* Make sure the base path is absolute, since only absolute paths are
           stored in the database. */
        if (realpath(base, real_base) == NULL) {
            fprintf(stderr, "realpath() failed: %s\n", strerror(errno));
            goto on_error;
        }

        if (brightness != -1) {
            if (asprintf(&query, "SELECT id FROM wallpapers WHERE path " \
                    "LIKE \"%s%%\" AND brightness=%d ORDER BY RANDOM() LIMIT %d;",
                    real_base, brightness, LIST_MAX) == -1) {
                fprintf(stderr, "asprintf() failed: %s\n", strerror(errno));
                goto on_error;
            }
        }
        else {
            if (asprintf(&query, "SELECT id FROM wallpapers WHERE path " \
                    "LIKE \"%s%%\" ORDER BY RANDOM() LIMIT %d;",
                    real_base, LIST_MAX) == -1) {
                fprintf(stderr, "asprintf() failed: %s\n", strerror(errno));
                goto on_error;
            }
        }

        rc = sqlite3_exec(db, query, callback_wallpaper_list, &wallpaper_list, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Error: Failed to execute query: %s\n", query);
            goto on_error;
        }

        wallpaper_list_populated = 1;
    }

    if (wallpaper_count == 0) {
        goto on_error;
    }

    // Get random index for wallpaper_list
    if (wallpaper_current == wallpaper_count) {
        wallpaper_current = 0;
    }

    id = wallpaper_list[wallpaper_current++];

    // Set the wallpaper path
    if (asprintf(&query2, "SELECT path FROM wallpapers WHERE id=%d;", id) == -1) {
        fprintf(stderr, "asprintf() failed: %s\n", strerror(errno));
        goto on_error;
    }

    rc = sqlite3_exec(db, query2, callback_wallpaper_path, &path, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: query or query callback failure: %s\n", query2);
        goto on_error;
    }

    // Deallocate
    if (query) {
        free(query);
    }

    if (query2) {
        free(query2);
    }

    return id;

on_error:
    if (query) {
        free(query);
    }

    if (query2) {
        free(query2);
    }

    return -1;
}

/**
  Callback for nextwall() which populates an array with wallpaper IDs.

  Populates the first argumenent to the callback.

  @param[out] param The wallpaper list array.
  @param[in] argc The number of columns in the result.
  @param[in] argv An array of pointers to strings obtained for each column.
  @param[in] colnames An array of pointers to strings where each entry
             represents the name of corresponding result column.
 */
int callback_wallpaper_list(void *param, int argc, char **argv, char **colnames) {
    int *wallpaper_list = (int *)param;
    wallpaper_list[wallpaper_count] = atoi(argv[0]);
    ++wallpaper_count;

    return 0;
}

/**
  Callback for nextwall() which returns the path of the selected wallpaper.

  Sets the first argument to the callback.

  @param[out] param The path of the selected wallpaper.
  @param[in] argc The number of columns in the result.
  @param[in] argv An array of pointers to strings obtained for each column.
  @param[in] colnames An array of pointers to strings where each entry
             represents the name of corresponding result column.
 */
int callback_wallpaper_path(void *param, int argc, char **argv, char **colnames) {
    char **path = (char **)param;

    if (strlcpy(*path, argv[0], PATH_MAX) >= PATH_MAX) {
        return -1;
    }

    return 0;
}

/**
  Move wallpaper to trash and remove it from the nextwall database.

  @param[in] db The database handler.
  @param[in] path Absolute path of the wallpaper.
  @return Returns 0 on successful completion, and -1 on error.
 */
int remove_wallpaper(sqlite3 *db, char *path) {
    char *query = NULL;

    if (asprintf(&query, "DELETE FROM wallpapers WHERE path='%s';", path) == -1) {
        fprintf(stderr, "asprintf() failed: %s\n", strerror(errno));
        return -1;
    }

    if (sqlite3_exec(db, query, NULL, NULL, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error: failed to execute query: %s\n", query);
        free(query);
        return -1;
    }

    free(query);

    // Move wallpaper to trash.
    if (file_trash(path) == -1) {
        return -1;
    }

    return 0;
}
