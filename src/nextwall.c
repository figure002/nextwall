#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cfgpath.h"        /* Get user paths */

char cfgpath[MAX_PATH];    /* Path to user configurations directory */
char dbfile[MAX_PATH];      /* Path to database file */

int main(int argc, char *argv[]) {
    struct stat sts;

    /* Set data directory */
    get_user_data_folder(cfgpath, sizeof(cfgpath), "nextwall");

    /* Set the database file path */
    strcpy(dbfile, cfgpath);
    strcat(dbfile, "nextwall.db");

    if (cfgpath[0] == 0) {
        printf("Unable to find home directory.\n");
        return 1;
    }

    // Create the data directory.
    if ( stat(cfgpath, &sts) != 0 || !S_ISDIR(sts.st_mode) ) {
        // Fail to get info about the file. Directory may not exist.
        printf("Creating directory %s...\n", cfgpath);
        if ( mkdir(cfgpath, 0755) == 0 ) {
            printf("Directory created.\n");
        }
        else {
            printf("Failed to create directory %s\n", cfgpath);
            return 1;
        }
    }
    else {
        printf("The directory %s exists.\n", cfgpath);
    }

    printf("Saving data to %s\n", dbfile);

    return 0;
}
