#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>
#include <dirent.h>
#include "cfgpath.h"        /* Get user paths */

#define MAXLINE 1000
#define NEXTWALL_DB_VERSION 0.2

char cfgpath[MAX_PATH]; /* Path to user configurations directory */
char dbfile[MAX_PATH]; /* Path to database file */
char default_wallpaper_path[] = "/usr/share/backgrounds/";
char *wallpaper_path;
int c, rc;
double latitude = 51.48, longitude = 0.0;

/* function prototypes */
int nextwall_make_db(sqlite3 *db);
int handle_sqlite_response(int rc);

/* example sqlite callback function
static int callback(void *notused, int argc, char **argv, char **colnames) {
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", colnames[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}
*/

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

void nextwall_scan_dir(const char *name, int recursive) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;
    if (!(entry = readdir(dir)))
        return;

    do {
        if (entry->d_type == DT_DIR) {
            if (!recursive)
                continue;
            char path[1024];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0  || strcmp(entry->d_name, "..") == 0 || \
                strcmp(entry->d_name, ".thumbs") == 0)
                continue;
            nextwall_scan_dir(path, recursive);
        }
        else {
            printf("%s/%s\n", name, entry->d_name);
        }
    } while ( (entry = readdir(dir)) );
    closedir(dir);
}

