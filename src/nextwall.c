#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfgpath.h"
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
    {"verbose", 'v', 0, 0, "Increase verbosity"},
    {"location", 'l', "LAT:LON", 0, "Specify latitude and longitude of your current location"},
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments {
    char *args[1]; /* PATH argument */
    int recursion, time, scan, verbose;
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
        case 'v':
            arguments->verbose = verbose = 1;
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
    int rc = -1, local_brightness = -1;
    int found, i, id;
    sqlite3 *db;
    GSettings *settings;

    /* Create a new GSettings object */
    settings = g_settings_new("org.gnome.desktop.background");

    /* Default argument values. */
    arguments.recursion = 0;
    arguments.time = 0;
    arguments.scan = 0;
    arguments.verbose = 0;
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

    /* Set data directory */
    get_user_data_folder(cfgpath, sizeof cfgpath, "nextwall");

    if (cfgpath[0] == 0) {
        fprintf(stderr, "Error: Unable to find home directory.\n");
        return 1;
    }

    /* Set the database file path */
    strcpy(dbfile, cfgpath);
    strcat(dbfile, "nextwall.db");

    /* Create the data directory if it doesn't exist. */
    if ( stat(cfgpath, &sts) != 0 || !S_ISDIR(sts.st_mode) ) {
        eprintf("Creating directory %s\n", cfgpath);
        if ( mkdir(cfgpath, 0755) == 0 ) {
            eprintf("Directory created.\n");
        }
        else {
            fprintf(stderr, "Error: Failed to create directory %s\n", cfgpath);
            return 1;
        }
    }

    // Create the database file if it doesn't exist.
    if ( stat(dbfile, &sts) != 0 ) {
        eprintf("Creating database... ");
        if ( (rc = sqlite3_open(dbfile, &db)) == 0 && nextwall_make_db(db) == 0 ) {
            eprintf("Done\n");
        }
        else {
            eprintf("Failed\n");
            fprintf(stderr, "Error: Creating database failed.\n");
            return 1;
        }
    }

    // Open database connection.
    if ( rc != SQLITE_OK ) {
        if ( (rc = sqlite3_open(dbfile, &db)) != 0 ) {
            fprintf(stderr, "Error: Can't open database: %s\n", sqlite3_errmsg(db));
            return 1;
        }
    }

    /* Search directory for wallpapers */
    if (arguments.scan) {
        fprintf(stderr, "Scanning for new wallpapers...");
        found = nextwall_scan_dir(db, wallpaper_dir, arguments.recursion);
        fprintf(stderr, " Done\n");
        fprintf(stderr, "Found %d new wallpapers\n", found);
        goto Return;
    }

    /* Get local brightness */
    if (arguments.time) {
        local_brightness = get_local_brightness();
        switch (local_brightness) {
            case 0:
                eprintf("Selecting wallpaper for night.\n");
                break;
            case 1:
                eprintf("Selecting wallpapers for twilight.\n");
                break;
            case 2:
                eprintf("Selecting wallpaper for day.\n");
                break;
            default:
                fprintf(stderr, "Error: Could not determine the local brightness value.\n");
                goto Return;
        }
    }

    /* Set wallpaper_path */
    if ( (id = nextwall(db, wallpaper_dir, local_brightness)) == -1 ) {
        fprintf(stderr, "No wallpapers found for directory %s. Try the --scan or --recursion option.\n", wallpaper_dir);
        goto Return;
    }

    /* Get the path of the current wallpaper */
    get_background_uri(settings, current_wallpaper);

    /* Make sure we select a different wallpaper */
    for (i = 0; strcmp(wallpaper_path, current_wallpaper) == 0; i++) {
        if (i == 3) {
            fprintf(stderr, "Not enough wallpapers found. Select a different directory or use the --scan option.\n");
            goto Return;
        }
        id = nextwall(db, wallpaper_dir, local_brightness);
    }

    /* Set the new wallpaper */
    eprintf("Setting wallpaper to %s\n", wallpaper_path);
    set_background_uri(settings, wallpaper_path);

    goto Return;

Return:
    g_object_unref(settings);
    sqlite3_close(db);
    return 0;
}

