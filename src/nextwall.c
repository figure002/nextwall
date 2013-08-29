#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sqlite3.h>
#include "cfgpath.h"        /* Get user paths */
#include "nextwall.h"       /* NextWall routines */

char cfgpath[MAX_PATH];     /* Path to user configurations directory */
char dbfile[MAX_PATH];      /* Path to database file */

int main(int argc, char *argv[]) {
    struct stat sts;
    sqlite3 *db;
    int rc = -1;

    /* Set data directory */
    get_user_data_folder(cfgpath, sizeof(cfgpath), "nextwall_test");

    /* Set the database file path */
    strcpy(dbfile, cfgpath);
    strcat(dbfile, "nextwall.db");

    if (cfgpath[0] == 0) {
        fprintf(stderr, "Unable to find home directory.\n");
        return 1;
    }

    // TODO: Check for `identify'

    // Create the data directory.
    if ( stat(cfgpath, &sts) != 0 || !S_ISDIR(sts.st_mode) ) {
        // Fail to get info about the file. Directory may not exist.
        printf("Creating directory %s\n", cfgpath);
        if ( mkdir(cfgpath, 0755) == 0 ) {
            printf("Directory created.\n");
        }
        else {
            fprintf(stderr, "Failed to create directory.\n");
            return 1;
        }
    }

    // Create the database file if it doesn't exist.
    if ( stat(dbfile, &sts) != 0 ) {
        printf("Creating database... ");
        if ( (rc = sqlite3_open(dbfile, &db)) == 0 ) {
            nextwall_make_db(db);
            printf("Done\n");
        }
        else {
            printf("Failed\n");
            fprintf(stderr, "Creating database failed: %s\n", sqlite3_errmsg(db));
            return 1;
        }
    }

    // Open database connection.
    if ( rc != SQLITE_OK ) {
        printf("Opening database... ");
        if ( (rc = sqlite3_open(dbfile, &db)) == 0 ) {
            printf("Done\n");
        }
        else {
            printf("Failed\n");
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            return 1;
        }
    }

    printf("Saving data to %s\n", dbfile);
    sqlite3_close(db);

    return 0;
}

