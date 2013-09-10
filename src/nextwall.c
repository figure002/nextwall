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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cfgpath.h"
#include "nextwall.h"

/* Set up the arguments parser */
const char *argp_program_version = PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Program documentation */
static char doc[] = "nextwall -- a wallpaper rotator";

/* A description of the arguments we accept */
static char args_doc[] = "PATH";

/* The options we understand */
static struct argp_option options[] = {
    {"recursion", 'r', 0, 0, "Find wallpapers in subdirectories"},
    {"brightness", 'b', "N", 0, "Force brightness value to night (0), twilight (1), or day (2)"},
    {"time", 't', 0, 0, "Find wallpapers that fit the time of day"},
    {"scan", 's', 0, 0, "Scan for images files in PATH"},
    {"verbose", 'v', 0, 0, "Increase verbosity"},
    {"location", 'l', "LAT:LON", 0, "Specify latitude and longitude of your current location"},
    { 0 }
};

/* Used by main to communicate with parse_opt */
struct arguments {
    char *args[1]; /* PATH argument */
    int recursion, brightness, time, scan, verbose;
    char *location;
};

/* Parse a single option */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;
    char tmp[50];
    char *lat, *lon;
    int rc, b;

    switch (key)
    {
        case 'r':
            arguments->recursion = 1;
            break;
        case 't':
            arguments->time = 1;
            break;
        case 'b':
            if (!isdigit(*arg)) {
                argp_usage(state);
                break;
            }

            b = atoi(arg);
            if ( !(b == 0 || b == 1 || b == 2) ) {
                argp_usage(state);
                break;
            }

            arguments->brightness = b;
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

            rc = sscanf(lat, "%lf", &latitude); // parse string to double
            if (rc == 0) {
                fprintf(stderr, "Incorrect value for latitude\n");
                argp_usage(state);
            }
            rc = sscanf(lon, "%lf", &longitude); // parse string to double
            if (rc == 0) {
                fprintf(stderr, "Incorrect value for longitude\n");
                argp_usage(state);
            }
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num >= 1) {
                 // Too many arguments.
                 argp_usage(state);
            }
            arguments->args[state->arg_num] = arg;
            break;

        case ARGP_KEY_END:
            if (state->arg_num == 0) {
                // Use the default wallpapers path if none specified.
                arguments->args[0] = default_wallpaper_dir;
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

/* Our argp parser */
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char **argv) {
    struct arguments arguments;
    struct stat sts;
    int rc = -1, local_brightness = -1, ann_found = 0;
    int found, i, j, id;
    sqlite3 *db;
    GSettings *settings;
    char *annfiles[3];
    char tmp[PATH_MAX];

    // Create a new GSettings object
    settings = g_settings_new("org.gnome.desktop.background");

    // Default argument values
    arguments.recursion = 0;
    arguments.brightness = -1;
    arguments.time = 0;
    arguments.scan = 0;
    arguments.verbose = 0;
    arguments.location = "51.48:0.0";

    /* Parse arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    // Set the wallpaper path
    wallpaper_dir = arguments.args[0];

    if ( stat(wallpaper_dir, &sts) != 0 || !S_ISDIR(sts.st_mode) ) {
        fprintf(stderr, "The wallpaper path %s doesn't exist.\n", wallpaper_dir);
        return 1;
    }

    // Set data directory
    get_user_data_folder(cfgpath, sizeof cfgpath, "nextwall");

    if (cfgpath[0] == 0) {
        fprintf(stderr, "Error: Unable to find home directory.\n");
        return 1;
    }

    // Set the database file path
    strcpy(dbfile, cfgpath);
    strcat(dbfile, "nextwall.db");

    // Set the ANN file path
    strcpy(tmp, cfgpath);
    strcat(tmp, "nextwall.net");
    annfiles[0] = tmp;
    annfiles[1] = "/usr/local/share/nextwall/nextwall.net";
    annfiles[2] = "/usr/share/nextwall/nextwall.net";

    for (j = 0; j < 3; j++) {
        if (file_exists(annfiles[j])) {
            annfile = annfiles[j];
            ann_found = 1;
            eprintf("Using ANN %s\n", annfile);
            break;
        }
    }

    if (!ann_found) {
        fprintf(stderr, "Error: Could not find ANN file nextwall.net\n");
        return 1;
    }

    // Create the data directory if it doesn't exist.
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
        if ( (rc = sqlite3_open(dbfile, &db)) == 0 && make_db(db) == 0 ) {
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

    // Search directory for wallpapers
    if (arguments.scan) {
        fprintf(stderr, "Scanning for new wallpapers...");
        found = scan_dir(db, wallpaper_dir, arguments.recursion);
        fprintf(stderr, " Done\n");
        fprintf(stderr, "Found %d new wallpapers\n", found);
        goto Return;
    }

    // Get local brightness
    if (arguments.time) {
        if (arguments.brightness == -1)
            local_brightness = get_local_brightness(latitude, longitude);
        else
            local_brightness = arguments.brightness;

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

    // Set wallpaper_path
    if ( (id = nextwall(db, wallpaper_dir, local_brightness)) == -1 ) {
        fprintf(stderr, "No wallpapers found for directory %s. Try the --scan or --recursion option.\n", wallpaper_dir);
        goto Return;
    }

    // Get the path of the current wallpaper
    get_background_uri(settings, current_wallpaper);

    // Make sure we select a different wallpaper
    for (i = 0; strcmp(wallpaper_path, current_wallpaper) == 0; i++) {
        if (i == 3) {
            fprintf(stderr, "Not enough wallpapers found. Select a different directory or use the --scan option.\n");
            goto Return;
        }
        id = nextwall(db, wallpaper_dir, local_brightness);
    }

    // Set the new wallpaper
    eprintf("Setting wallpaper to %s\n", wallpaper_path);
    set_background_uri(settings, wallpaper_path);

    goto Return;

Return:
    g_object_unref(settings);
    sqlite3_close(db);
    return 0;
}

