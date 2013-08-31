#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>
#include <dirent.h>
#include <limits.h>
#include <magic.h>
#include "cfgpath.h"        /* Get user paths */

#define MAXLINE 2000
#define NEXTWALL_DB_VERSION 0.2

char cfgpath[MAX_PATH]; /* Path to user configurations directory */
char dbfile[MAX_PATH]; /* Path to database file */
char default_wallpaper_path[] = "/usr/share/backgrounds/";
char *wallpaper_path;
int c, rc, known_image;
double latitude = 51.48, longitude = 0.0;

/* function prototypes */
int nextwall_make_db(sqlite3 *db);
int handle_sqlite_response(int rc);
void nextwall_scan_dir(sqlite3 *db, const char *name, int recursive);
int nextwall_is_known_image(sqlite3 *db, const char *path);
int known_image_callback(void *notused, int argc, char **argv, char **colnames);
int nextwall_save_image_info(sqlite3 *db, const char *path);

/* Print the SQLite error message if there was an error. */
int handle_sqlite_response(int rc) {
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", "Unknown");
        exit(1);
    }
    return 0;
}

/* Create an empty database with the necessary tables.
Returns 0 on success, -1 on failure. */
int nextwall_make_db(sqlite3 *db) {
    char *sql;
    char sqlstr[MAXLINE];

    sql = "CREATE TABLE wallpapers (" \
        "id INTEGER PRIMARY KEY," \
        "path TEXT," \
        "kurtosis FLOAT," \
        "defined_brightness INTEGER," \
        "rating INTEGER);";
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

    return 0;
}

void nextwall_scan_dir(sqlite3 *db, const char *name, int recursive) {
    DIR *dir;
    struct dirent *entry;

    /* Initialize Magic Number Recognition Library */
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic, NULL);

    if (!(dir = opendir(name)))
        return;
    if (!(entry = readdir(dir)))
        return;

    do {
        char tmp[PATH_MAX];
        char path[PATH_MAX];
        snprintf(tmp, sizeof(tmp), "%s/%s", name, entry->d_name);
        realpath(tmp, path);

        if (entry->d_type == DT_DIR) {
            if (!recursive)
                continue;
            if (strcmp(entry->d_name, ".") == 0  || strcmp(entry->d_name, "..") == 0 || \
                strcmp(entry->d_name, ".thumbs") == 0)
                continue;
            nextwall_scan_dir(db, path, recursive);
        }
        else if (strstr(magic_file(magic, path), "image")) {
            if (nextwall_is_known_image(db, path))
                continue;

            if (nextwall_save_image_info(db, path) == 0) {
                fprintf(stderr, "  Found %s\n", path);
            }
            else {
                fprintf(stderr, "Failed to save image info for %s\n", path);
                goto Return;
            }
        }
    } while ( (entry = readdir(dir)) );

    goto Return;

Return:
    magic_close(magic);
    closedir(dir);
}

int nextwall_is_known_image(sqlite3 *db, const char *path) {
    char query[] = "SELECT kurtosis FROM wallpapers WHERE path='%s';";
    char sql[MAXLINE];
    known_image = 0;

    snprintf(sql, sizeof(sql), query, path);
    rc = sqlite3_exec(db, sql, known_image_callback, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to execute query: %s\n", sql);
        exit(1);
    }
    return known_image;
}

int known_image_callback(void *notused, int argc, char **argv, char **colnames) {
    known_image = 1;
    return 0;
}

int nextwall_save_image_info(sqlite3 *db, const char *path) {
    char query[] = "INSERT INTO wallpapers VALUES (null, '%s', null, null, null);";
    char sql[MAXLINE];

    snprintf(sql, sizeof(sql), query, path);
    rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    if (rc != SQLITE_OK)
        return -1;
    return 0;
}

