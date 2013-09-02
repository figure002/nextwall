#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sqlite3.h>
#include <argp.h>
#include "nextwall.h"

const char *argp_program_version = "0.4.0";
const char *argp_program_bug_address = "<serrano.pereira@gmail.com>";

/* Program documentation. */
static char doc[] = "nextwall -- a wallpaper rotator";

/* A description of the arguments we accept. */
static char args_doc[] = "PATH";

/* The options we understand. */
static struct argp_option options[] = {
    {"recursion", 'r', 0, 0, "Find wallpapers in subdirectories"},
    {"time", 't', 0, 0, "Find wallpapers that fit the time of day"},
    {"scan", 's', 0, 0, "Scan for images files in PATH"},
    {"location", 'l', "LAT:LON", 0, "Specify latitude and longitude of your current location"},
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments {
    char *args[1]; /* PATH argument */
    int recursion, time, scan;
    char *location;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;
    char tmp[50];
    char *lat, *lon;
    int rc;

    switch (key)
    {
        case 'r':
            arguments->recursion = 1;
            break;
        case 't':
            arguments->time = 1;
            break;
        case 's':
            arguments->scan = 1;
            break;
        case 'l':
            arguments->location = arg;

            strcpy(tmp, arg);
            if (strstr(tmp, ":") == NULL) {
                fprintf(stderr, "Incorrect value for location\n");
                argp_usage(state);
            }
            lat = strtok(tmp, ":");
            lon = strtok(NULL, ":");

            rc = sscanf(lat, "%lf", &latitude); /* parse string to double */
            if (rc == 0) {
                fprintf(stderr, "Incorrect value for latitude\n");
                argp_usage(state);
            }
            rc = sscanf(lon, "%lf", &longitude); /* parse string to double */
            if (rc == 0) {
                fprintf(stderr, "Incorrect value for longitude\n");
                argp_usage(state);
            }
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num >= 1) {
                 /* Too many arguments. */
                 argp_usage(state);
            }
            arguments->args[state->arg_num] = arg;
            break;

        case ARGP_KEY_END:
            if (state->arg_num == 0) {
                /* Use the default wallpapers path if none specified. */
                arguments->args[0] = default_wallpaper_dir;
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char **argv) {
    struct arguments arguments;
    struct stat sts;
    int rc = -1;
    int found, i, id;
    sqlite3 *db;

    /* Default argument values. */
    arguments.recursion = 0;
    arguments.time = 0;
    arguments.scan = 0;
    arguments.location = "51.48:0.0";

    /* Parse arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    /* Set the wallpaper path */
    wallpaper_dir = arguments.args[0];

    if ( stat(wallpaper_dir, &sts) != 0 || !S_ISDIR(sts.st_mode) ) {
        fprintf(stderr, "The wallpaper path %s doesn't exist.\n", wallpaper_dir);
        return 1;
    }

    /*printf("PATH = %s\n"
        "RECURSION = %s\nTIME = %s\n"
        "SCAN = %s\n"
        "LAT = %f\nLON = %f\n",
        arguments.args[0],
        arguments.recursion ? "yes" : "no",
        arguments.time ? "yes" : "no",
        arguments.scan ? "yes" : "no",
        latitude,
        longitude);*/

    /* Set data directory */
    get_user_data_folder(cfgpath, sizeof(cfgpath), "nextwall_test");

    if (cfgpath[0] == 0) {
        fprintf(stderr, "Unable to find home directory.\n");
        return 1;
    }

    /* Set the database file path */
    strcpy(dbfile, cfgpath);
    strcat(dbfile, "nextwall.db");

    // TODO: Check for `identify'

    /* Create the data directory if it doesn't exist. */
    if ( stat(cfgpath, &sts) != 0 || !S_ISDIR(sts.st_mode) ) {
        fprintf(stderr, "Creating directory %s\n", cfgpath);
        if ( mkdir(cfgpath, 0755) == 0 ) {
            fprintf(stderr, "Directory created.\n");
        }
        else {
            fprintf(stderr, "Failed to create directory.\n");
            return 1;
        }
    }

    // Create the database file if it doesn't exist.
    if ( stat(dbfile, &sts) != 0 ) {
        fprintf(stderr, "Creating database... ");
        if ( (rc = sqlite3_open(dbfile, &db)) == 0 && nextwall_make_db(db) == 0 ) {
            fprintf(stderr, "Done\n");
        }
        else {
            fprintf(stderr, "Failed\n");
            fprintf(stderr, "Creating database failed.\n");
            return 1;
        }
    }

    // Open database connection.
    if ( rc != SQLITE_OK ) {
        if ( (rc = sqlite3_open(dbfile, &db)) != 0 ) {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            return 1;
        }
    }

    /* Search directory for wallpapers */
    if (arguments.scan) {
        fprintf(stderr, "Scanning for new wallpapers...\n");
        found = nextwall_scan_dir(db, wallpaper_dir, arguments.recursion);
        fprintf(stderr, "\nDone (found %d new wallpapers)\n", found);
        goto Return;
    }

    /* Get the path of the current wallpaper */
    get_background_uri(current_wallpaper);
    fprintf(stderr, "Before: %s\n", current_wallpaper);

    /* Set wallpaper_path */
    if ( (id = nextwall(db, wallpaper_dir)) == -1 ) {
        fprintf(stderr, "No wallpapers found for directory %s. Try the --scan or --recursion option.\n", wallpaper_dir);
        goto Return;
    }

    /* Make sure we select a different wallpaper */
    for (i = 0; wallpaper_path == current_wallpaper; i++) {
        if (i == 3) {
            fprintf(stderr, "Not enough wallpapers found. Select a different directory or use the --scan option.");
            goto Return;
        }
        id = nextwall(db, wallpaper_dir);
    }

    /* Set the new wallpaper */
    fprintf(stderr, "Setting wallpaper to %s ... ", wallpaper_path);
    if (set_background_uri(wallpaper_path)) {
        fprintf(stderr, "Done\n");
        get_background_uri(current_wallpaper);
        fprintf(stderr, "After: %s\n", current_wallpaper);
    }
    else {
        fprintf(stderr, "Failed\n");
    }

    goto Return;

Return:
    sqlite3_close(db);
    return 0;
}
