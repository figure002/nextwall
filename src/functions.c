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

#define _GNU_SOURCE   /* for tm_gmtoff, tm_zone, and asprintf() */

#include <dirent.h>
#include <fcntl.h>
#include <floatfann.h>
#include <limits.h>
#include <magic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>

#include "config.h"
#include "gnome.h"
#include "image.h"
#include "nextwall.h"
#include "std.h"
#include "sunriset.h"

static int wallpaper_list_populated = 0;
static int max_walls = 0;
static int rc;
static int wallpaper_list[LIST_MAX];
static int rand_seeded = 0;

/* Function prototypes */
static int save_image_info(sqlite3_stmt *stmt, struct fann *ann, const char *path);
static int get_brightness(struct fann *ann, double kurtosis, double lightness);
static int is_known_image(sqlite3 *db, const char *path);
static int callback_known_image(void *param, int argc, char **argv, char **colnames);
static int callback_wallpaper_list(void *param, int argc, char **argv, char **colnames);
static int callback_wallpaper_path(void *param, int argc, char **argv, char **colnames);


/**
  Create a new nextwall database.

  @param[in] db The database handler.
  @return Returns 0 on success, -1 on failure.
 */
int make_db(sqlite3 *db) {
    int rc = 0, rc2 = 0;
    char *query;
    char *aquery = NULL;

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

    if ((rc2 = asprintf(&aquery, "INSERT INTO info VALUES (null, 'version', %f);",
                NEXTWALL_DB_VERSION)) == -1) {
        fprintf(stderr, "Error: Failed to concatenate string\n");
        goto Return;
    }

    rc = sqlite3_exec(db, aquery, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        goto Return;

    rc = sqlite3_exec(db, "CREATE UNIQUE INDEX wallpapers_path_idx ON wallpapers (path);",
            NULL, NULL, NULL);

    goto Return;

Return:
    if (aquery)
        free(aquery);

    if (rc2 == -1)
        rc = -1;

    return rc;
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

    if (recursive < 2)
        sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    if (!(dir = opendir(base)))
        return found;
    if (!(entry = readdir(dir)))
        return found;

    // Prepare INSERT statement
    query = "INSERT INTO wallpapers VALUES (null, @PATH, @KUR, @LGT, @BRI);";
    sqlite3_prepare_v2(db, query, strlen(query)+1, &stmt, &tail);

    do {
        if (asprintf(&path_tmp, "%s/%s", base, entry->d_name) == -1) {
            fprintf(stderr, "Error: asprintf() failed\n");
            goto Return;
        }

        if (realpath(path_tmp, path) == NULL) {
            fprintf(stderr, "Error: realpath() failed\n");
            goto Return;
        }

        if (entry->d_type == DT_DIR) {
            if (!recursive)
                continue;
            if (strcmp(entry->d_name, ".") == 0  || strcmp(entry->d_name, "..") == 0 || \
                strcmp(entry->d_name, ".thumbs") == 0)
                continue;
            found += scan_dir(db, path, ann, recursive+1);
        }
        else if (strstr(magic_file(magic, path), "image")) {
            if (is_known_image(db, path))
                continue;

            if (save_image_info(stmt, ann, path) == 0) {
                ++found;
                if (found % 10 == 0) {
                    fprintf(stderr, ".");
                }
            }
            else {
                fprintf(stderr, "Error: Failed to save image info for %s\n", path);
                goto Return;
            }
        }
    } while ( (entry = readdir(dir)) );

    goto Return;

Return:
    if (recursive < 2) {
        sqlite3_exec(db, "END TRANSACTION", NULL, NULL, NULL);
        sqlite3_finalize(stmt);
    }
    magic_close(magic);
    if (path_tmp)
        free(path_tmp);
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
    char *query = NULL;

    if (asprintf(&query, "SELECT id FROM wallpapers WHERE path='%s';", path) == -1) {
        fprintf(stderr, "Error: Failed to concatenate string\n");
        goto Error;
    }

    rc = sqlite3_exec(db, query, callback_known_image, &known, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", query);
        goto Error;
    }

    free(query);
    return known;

Error:
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

    // Get the lightness for this image
    if (get_image_info(path, &kurtosis, &lightness) == -1)
        return -1;

    // Get image brigthness
    brightness = get_brightness(ann, kurtosis, lightness);

    // Bind values to prepared statement
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, kurtosis);
    sqlite3_bind_double(stmt, 3, lightness);
    sqlite3_bind_int(stmt, 4, brightness);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        return -1;
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    return 0;
}

/**
  Calculate the brightness value.

  Uses the Artificial Neural Network to define the brightness value
  for a given kurtosis and lightness.

  @param[in] The Artificial Neural Network.
  @param[in] The kurtosis value of the wallpaper.
  @param[in] The lightness value of the wallpaper.
  @return Returns the brightness value (0 for dark, 1 for intermediate, 2 for
          light). Returns -1 on failure.
 */
int get_brightness(struct fann *ann, double kurtosis, double lightness) {
    fann_type *out;
    fann_type input[2];
    float diff[3];
    int i;

    input[0] = kurtosis;
    input[1] = lightness;
    out = fann_run(ann, input);

    diff[0] = 1.0 - out[0];
    diff[1] = 1.0 - out[1];
    diff[2] = 1.0 - out[2];

    qsort(diff, 3, sizeof (float), floatcmp);

    for (i = 0; i < 3; i++) {
        if ((1.0 - out[i]) == diff[0]) {
            return i;
        }
    }

    return -1;
}

/**
  Select a random wallpaper from the nextwall database.

  It sets the value of `wallpaper_path` to the absolute path of the randomly
  selected wallpaper.

  @param[in] db The database handler.
  @param[in] path The base directory from which to select wallpapers.
  @param[in] brightness If set to 0, 1, or 2, wallpapers matching this
             brightness value are returned.
  @param[out] wallpaper Will be set to the path of the next wallpaper.
  @return Returns the ID of a randomly selected wallpaper on success, -1
          otherwise.
 */
int nextwall(sqlite3 *db, const char *base, int brightness, char **wallpaper) {
    int i;
    unsigned seed;
    char *query = NULL;
    char *query2 = NULL;

    if (!wallpaper_list_populated) {
        if (brightness != -1) {
            if (asprintf(&query, "SELECT id FROM wallpapers WHERE path " \
                    "LIKE \"%s%%\" AND brightness=%d ORDER BY RANDOM() LIMIT %d;",
                    base, brightness, LIST_MAX) == -1) {
                fprintf(stderr, "Error: asprintf() failed\n");
                goto Error;
            }
        }
        else {
            if (asprintf(&query, "SELECT id FROM wallpapers WHERE path " \
                    "LIKE \"%s%%\" ORDER BY RANDOM() LIMIT %d;",
                    base, LIST_MAX) == -1) {
                fprintf(stderr, "Error: asprintf() failed\n");
                goto Error;
            }
        }

        rc = sqlite3_exec(db, query, callback_wallpaper_list, &wallpaper_list, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Error: Failed to execute query: %s\n", query);
            goto Error;
        }

        wallpaper_list_populated = 1;
    }

    if (max_walls == 0)
        goto Error;

    // Set the seed for the random number generator
    if (!rand_seeded) {
        if (read(open("/dev/urandom", O_RDONLY), &seed, sizeof seed) == -1)
            goto Error;

        srand(seed);
        rand_seeded = 1;
    }

    // Get random index for wallpaper_list
    i = rand() % max_walls;

    // Set the wallpaper path
    if (asprintf(&query2, "SELECT path FROM wallpapers WHERE id=%d;",
            wallpaper_list[i]) == -1) {
        fprintf(stderr, "Error: asprintf() failed\n");
        goto Error;
    }

    rc = sqlite3_exec(db, query2, callback_wallpaper_path, wallpaper, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", query2);
        goto Error;
    }

    // Deallocate
    if (query)
        free(query);
    if (query2)
        free(query2);

    return wallpaper_list[i];

Error:
    if (query)
        free(query);
    if (query2)
        free(query2);
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
    wallpaper_list[max_walls] = atoi(argv[0]);
    ++max_walls;
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
    //*path = (char *) realloc(*path, sizeof(argv[0]));
    strcpy(*path, argv[0]);
    //strncpy(*path, argv[0], sizeof(*path));
    return 0;
}

/**
  Return the local brightness value.

  This function uses sunriset.h to calculate sunrise, sunset, and civil
  twilight times.

  @param[in] lat The latitude of the current location.
  @param[in] lon The longitude of the current location.
  @return Returns 0 for night, 1 for twilight, or 2 for day, depending on
          local time. Returns -1 if brightness could not be defined.
 */
int get_local_brightness(double lat, double lon) {
    struct tm ltime;
    time_t now;
    double htime, sunrise, sunset, civ_start, civ_end;
    int year, month, day, gmt_offset_h, rs, civ;
    char sr_s[6], ss_s[6], civ_start_s[6], civ_end_s[6];

    time(&now);
    localtime_r(&now, &ltime);
    year = ltime.tm_year + 1900;
    month = ltime.tm_mon + 1;
    day = ltime.tm_mday;
    gmt_offset_h = ltime.tm_gmtoff / 3600; // GMT offset with local time zone
    htime = (double)ltime.tm_hour + (double)ltime.tm_min / 60.0; // Current time in hours

    /* Set local sunrise, sunset, and civil twilight times */
    rs = sun_rise_set(year, month, day, lon, lat, &sunrise, &sunset);
    civ  = civil_twilight(year, month, day, lon, lat, &civ_start, &civ_end);

    switch (rs) {
        case 0:
            sunrise += gmt_offset_h;
            sunset += gmt_offset_h;
            eprintf("Sun rises %s, sets %s %s\n", hours_to_hm(sunrise, sr_s),
                    hours_to_hm(sunset, ss_s), ltime.tm_zone);
            break;
        case +1:
            eprintf("Sun above horizon\n");
            return 2; // Day
            break;
        case -1:
            eprintf("Sun below horizon\n");
            return 0; // Night
            break;
    }

    switch (civ) {
        case 0:
            civ_start += gmt_offset_h;
            civ_end += gmt_offset_h;
            eprintf("Civil twilight starts %s, ends %s %s\n",
                    hours_to_hm(civ_start, civ_start_s),
                    hours_to_hm(civ_end, civ_end_s), ltime.tm_zone);
            break;
        case +1:
            eprintf("Never darker than civil twilight\n");
            break;
        case -1:
            eprintf("Never as bright as civil twilight\n");
            break;
    }

    if (rs == 0 && civ == 0) {
        if (sunrise < htime && htime < sunset)
            return 2; // Day
        else if (htime < civ_start || htime > civ_end)
            return 0; // Night
        else
            return 1; // Twilight
    }
    else {
        return -1;
    }
}

/**
  Move wallpaper to trash and remove it from the nextwall database.

  @param[in] db The database handler.
  @param[in] path Absolute path of the wallpaper.
  @return Returns 0 on successful completion, and -1 on error.
 */
int remove_wallpaper(sqlite3 *db, char *path) {
    int rc = 0;
    char *query = NULL;

    if (asprintf(&query, "DELETE FROM wallpapers WHERE path='%s';", path) == -1) {
        fprintf(stderr, "Error: asprintf() failed\n");
        return -1;
    }

    rc = sqlite3_exec(db, query, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", query);
        free(query);
        return -1;
    }

    free(query);

    // Move wallpaper to trash.
    if ( file_trash(path) == 0 )
        rc = -1;

    return rc;
}

