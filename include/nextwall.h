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

#define BUFFER_SIZE 512
#define LIST_MAX 1000
#define NEXTWALL_DB_VERSION 0.2

char cfgpath[PATH_MAX]; /* Path to user configurations directory */
char dbfile[PATH_MAX]; /* Path to database file */
char default_wallpaper_dir[] = "/usr/share/backgrounds/";
char current_wallpaper[PATH_MAX];
char *wallpaper_dir, *wallpaper_path;
int c, rc, known_image, max_walls = 0;
double latitude = 51.48, longitude = 0.0;
int wallpaper_list[LIST_MAX];

/* function prototypes */
int nextwall_make_db(sqlite3 *db);
int handle_sqlite_response(int rc);
int nextwall_scan_dir(sqlite3 *db, const char *name, int recursive);
int nextwall_is_known_image(sqlite3 *db, const char *path);
int known_image_callback(void *notused, int argc, char **argv, char **colnames);
int nextwall_save_image_info(sqlite3_stmt *stmt, const char *path);
int nextwall(sqlite3 *db, const char *path, int brightness);
int nextwall_callback1(void *notused, int argc, char **argv, char **colnames);
int nextwall_callback2(void *notused, int argc, char **argv, char **colnames);
void set_background_uri(GSettings *settings, const char *path);
int get_background_uri(GSettings *settings, char *dest);
int get_local_brightness(struct tm *t, double rise, double set, double civ_start, double civ_end);

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
    char sqlstr[BUFFER_SIZE];

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

int nextwall_scan_dir(sqlite3 *db, const char *name, int recursive) {
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
    sprintf(sql, "INSERT INTO wallpapers VALUES (null, @PATH, null, null, null);");
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
            found += nextwall_scan_dir(db, path, recursive+1);
        }
        else if (strstr(magic_file(magic, path), "image")) {
            if (nextwall_is_known_image(db, path))
                continue;

            if (nextwall_save_image_info(stmt, path) == 0) {
                fprintf(stderr, "  Found %s\r", path);
                fflush(stderr);
                ++found;
            }
            else {
                fprintf(stderr, "Failed to save image info for %s\n", path);
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

int nextwall_is_known_image(sqlite3 *db, const char *path) {
    char query[] = "SELECT kurtosis FROM wallpapers WHERE path='%s';";
    char sql[strlen(query)+strlen(path)];
    known_image = 0;

    snprintf(sql, sizeof sql, query, path);
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

int nextwall_save_image_info(sqlite3_stmt *stmt, const char *path) {
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
        return -1;
    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    return 0;
}

int nextwall(sqlite3 *db, const char *path, int brightness) {
    char sql[BUFFER_SIZE] = "\0";
    int i;
    unsigned seed;

    if (brightness >= 0)
        snprintf(sql, sizeof sql, "SELECT id FROM wallpapers WHERE path LIKE \"%s%%\" AND defined_brightness=%d;", path, brightness);
    else
        snprintf(sql, sizeof sql, "SELECT id FROM wallpapers WHERE path LIKE \"%s%%\";", path);

    rc = sqlite3_exec(db, sql, nextwall_callback1, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to execute query: %s\n", sql);
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
        fprintf(stderr, "Failed to execute query: %s\n", sql);
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
    wallpaper_path = argv[0];
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
   local time t.*/
int get_local_brightness(struct tm *t, double rise, double set, double civ_start, double civ_end) {
    double time = (double)(*t).tm_hour + (double)(*t).tm_min / 60.0;

    if (rise < time || time < set)
        return 2; // Day
    else if (time < civ_start || time > civ_end)
        return 0; // Night
    else
        return 1; // Twilight
}

