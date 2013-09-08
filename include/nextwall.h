#define _GNU_SOURCE /* for tm_gmtoff and tm_zone */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <dirent.h>
#include <limits.h>
#include <magic.h>
#include <argp.h>
#include <time.h>
#include <fcntl.h> /* for open() */
#include <gio/gio.h>
#include <wand/MagickWand.h>

#include "sunriset.h"

#define BUFFER_SIZE 512
#define LIST_MAX 1000
#define NEXTWALL_DB_VERSION 0.3

#define eprintf(format, ...) do { \
    if (verbose) \
        fprintf(stderr, format, ##__VA_ARGS__); \
} while(0)

char cfgpath[PATH_MAX]; /* Path to user configurations directory */
char dbfile[PATH_MAX]; /* Path to database file */
char default_wallpaper_dir[] = "/usr/share/backgrounds/";
char current_wallpaper[PATH_MAX];
char *wallpaper_dir;
char wallpaper_path[PATH_MAX];
int verbose = 0, c, rc, known_image, max_walls = 0;
double latitude = 51.48, longitude = 0.0;
int wallpaper_list[LIST_MAX];
double kurtosis_values[4];

/* function prototypes */
int make_db(sqlite3 *db);
int scan_dir(sqlite3 *db, const char *name, int recursive);
int is_known_image(sqlite3 *db, const char *path);
int known_image_callback(void *notused, int argc, char **argv, char **colnames);
int save_image_info(sqlite3_stmt *stmt, const char *path);
int nextwall(sqlite3 *db, const char *path, int brightness);
int nextwall_callback1(void *notused, int argc, char **argv, char **colnames);
int nextwall_callback2(void *notused, int argc, char **argv, char **colnames);
void set_background_uri(GSettings *settings, const char *path);
int get_background_uri(GSettings *settings, char *dest);
int get_local_brightness(void);
char *hours_to_hm(double hours, char *s);
int get_image_info(const char *path, double *hue, double *saturation, double *lightness);

/* Create an empty database with the necessary tables.
Returns 0 on success, -1 on failure. */
int make_db(sqlite3 *db) {
    char *sql;
    char sqlstr[BUFFER_SIZE];

    sql = "CREATE TABLE wallpapers (" \
        "id INTEGER PRIMARY KEY," \
        "path TEXT," \
        "hue FLOAT," \
        "saturation FLOAT," \
        "lightness FLOAT," \
        "brightness INTEGER" \
        ");";
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        return -1;

    sql = "CREATE TABLE info (" \
        "id INTEGER PRIMARY KEY," \
        "name VARCHAR," \
        "value VARCHAR);";
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        return -1;

    sprintf(sqlstr, "INSERT INTO info VALUES (null, 'version', %f);", NEXTWALL_DB_VERSION);
    rc = sqlite3_exec(db, sqlstr, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        return -1;

    rc = sqlite3_exec(db, "CREATE UNIQUE INDEX wallpaper_path ON wallpapers (path);", NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        return -1;

    return 0;
}

int scan_dir(sqlite3 *db, const char *name, int recursive) {
    DIR *dir;
    struct dirent *entry;
    int found = 0;
    char sql[BUFFER_SIZE] = "\0";
    sqlite3_stmt *stmt;
    const char *tail = 0;

    /* Initialize Magic Number Recognition Library */
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic, NULL);

    sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    if (!(dir = opendir(name)))
        return found;
    if (!(entry = readdir(dir)))
        return found;

    /* Prepare INSERT statement */
    sprintf(sql, "INSERT INTO wallpapers VALUES (null, @PATH, @HUE, @SAT, @LGT, @BRI);");
    sqlite3_prepare_v2(db, sql, BUFFER_SIZE, &stmt, &tail);


    do {
        char tmp[PATH_MAX];
        char path[PATH_MAX];
        snprintf(tmp, sizeof tmp, "%s/%s", name, entry->d_name);
        realpath(tmp, path);

        if (entry->d_type == DT_DIR) {
            if (!recursive)
                continue;
            if (strcmp(entry->d_name, ".") == 0  || strcmp(entry->d_name, "..") == 0 || \
                strcmp(entry->d_name, ".thumbs") == 0)
                continue;
            found += scan_dir(db, path, recursive+1);
        }
        else if (strstr(magic_file(magic, path), "image")) {
            if (is_known_image(db, path))
                continue;

            if (save_image_info(stmt, path) == 0) {
                ++found;
                if (found % 10 == 0) {
                    fprintf(stderr, ".");
                    fflush(stderr);
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
    closedir(dir);
    return found;
}

int is_known_image(sqlite3 *db, const char *path) {
    char query[] = "SELECT id FROM wallpapers WHERE path='%s';";
    char sql[strlen(query)+strlen(path)];
    known_image = 0;

    snprintf(sql, sizeof sql, query, path);
    rc = sqlite3_exec(db, sql, known_image_callback, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", sql);
        exit(1);
    }
    return known_image;
}

int known_image_callback(void *notused, int argc, char **argv, char **colnames) {
    known_image = 1;
    return 0;
}

int save_image_info(sqlite3_stmt *stmt, const char *path) {
    double h,s,l;

    // Get the lightness for this image
    if (get_image_info(path, &h, &s, &l) == -1)
        return -1;

    // Bind values to prepared statement
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, h);
    sqlite3_bind_double(stmt, 3, s);
    sqlite3_bind_double(stmt, 4, l);
    sqlite3_bind_null(stmt, 5);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        return -1;
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    return 0;
}

/* Returns the image information */
int get_image_info(const char *path, double *hue, double *saturation, double *lightness) {
    MagickBooleanType status;
    MagickWand *magick_wand;
    PixelWand *pixel_wand;
    //Quantum red, green, blue;
    //size_t qrange = 0;

    MagickWandGenesis();
    magick_wand = NewMagickWand();
    pixel_wand = NewPixelWand();

    // Read the image
    status = MagickReadImage(magick_wand, path);
    if (status == MagickFalse)
        return -1;

    // Resize the image to 1x1 pixel (results in average color)
    MagickResizeImage(magick_wand, 1, 1, MitchellFilter, 1);

    // Get pixel color
    status = MagickGetImagePixelColor(magick_wand, 0, 0, pixel_wand);
    if (status == MagickFalse) {
        return -1;
    }

    // Get the color and brightness values
    PixelGetHSL(pixel_wand, hue, saturation, lightness);
    //red = PixelGetRedQuantum(pixel_wand);
    //green = PixelGetGreenQuantum(pixel_wand);
    //blue = PixelGetBlueQuantum(pixel_wand);
    //MagickGetQuantumRange(&qrange);
    //printf("R: %f, G: %f, B: %f, L: %.2f\n", red/(double)qrange, green/(double)qrange, blue/(double)qrange, *lightness);

    magick_wand = DestroyMagickWand(magick_wand);
    MagickWandTerminus();

    return 0;
}

int nextwall(sqlite3 *db, const char *path, int brightness) {
    char sql[BUFFER_SIZE] = "\0";
    int i;
    unsigned seed;

    if (brightness == 0)
        snprintf(sql, sizeof sql, "SELECT id FROM wallpapers WHERE path LIKE \"%s%%\" AND lightness <= 0.30;", path);
    else if (brightness == 1)
        snprintf(sql, sizeof sql, "SELECT id FROM wallpapers WHERE path LIKE \"%s%%\" AND lightness > 0.30 AND lightness < 0.50;", path);
    else if (brightness == 2)
        snprintf(sql, sizeof sql, "SELECT id FROM wallpapers WHERE path LIKE \"%s%%\" AND lightness >= 0.50;", path);
    else
        snprintf(sql, sizeof sql, "SELECT id FROM wallpapers WHERE path LIKE \"%s%%\";", path);

    rc = sqlite3_exec(db, sql, nextwall_callback1, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", sql);
        exit(1);
    }

    if (max_walls == 0)
        return -1;

    /* Set the seed for the random number generator */
    read(open("/dev/urandom", O_RDONLY), &seed, sizeof seed);
    srand(seed);

    /* Get random index for wallpaper_list */
    i = rand() % max_walls;

    /* Set the wallpaper path. */
    snprintf(sql, sizeof sql, "SELECT path FROM wallpapers WHERE id=%d;", wallpaper_list[i]);
    rc = sqlite3_exec(db, sql, nextwall_callback2, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", sql);
        exit(1);
    }

    return wallpaper_list[i];
}

int nextwall_callback1(void *notused, int argc, char **argv, char **colnames) {
    wallpaper_list[max_walls] = atoi(argv[0]);
    ++max_walls;
    return 0;
}

int nextwall_callback2(void *notused, int argc, char **argv, char **colnames) {
    strncpy(wallpaper_path, argv[0], sizeof wallpaper_path);
    return 0;
}

void set_background_uri(GSettings *settings, const char *path) {
    char pathc[PATH_MAX];

    if (!strstr(path, "file://"))
        sprintf(pathc, "file://%s", path);
    else
        strcpy(pathc, path);

    g_assert(g_settings_set(settings, "picture-uri", "s", pathc));
    g_settings_sync(); /* Make sure the changes are written to disk */
    g_assert_cmpstr(g_settings_get_string(settings, "picture-uri"), ==, pathc);
}

int get_background_uri(GSettings *settings, char *dest) {
    const char *uri;

    uri = g_variant_get_string(g_settings_get_value(settings, "picture-uri"), NULL);
    strcpy(dest, uri+7);
    return 0;
}

/* Returns 0 for night, 1 for twilight, or 2 for day, depending on
   local time t. Returns -1 if brightness could not be defined. */
int get_local_brightness(void) {
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
    rs = sun_rise_set(year, month, day, longitude, latitude, &sunrise, &sunset);
    civ  = civil_twilight(year, month, day, longitude, latitude, &civ_start, &civ_end);

    switch (rs) {
        case 0:
            sunrise += gmt_offset_h;
            sunset += gmt_offset_h;
            eprintf("Sun rises %s, sets %s %s\n", hours_to_hm(sunrise, sr_s), hours_to_hm(sunset, ss_s), ltime.tm_zone);
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
            eprintf("Civil twilight starts %s, ends %s %s\n", hours_to_hm(civ_start, civ_start_s), hours_to_hm(civ_end, civ_end_s), ltime.tm_zone);
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

char *hours_to_hm(double hours, char *dest) {
    char hm[6];
    double h = floor(hours);
    double m = (hours - h) * 60.0;
    snprintf(hm, sizeof hm, "%02.0f:%02.0f", h, m);
    strcpy(dest, hm);
    return dest;
}

