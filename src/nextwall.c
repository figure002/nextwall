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
#include <argp.h>

#include "cfgpath.h"
#include "nextwall.h"

/* Function prototypes */
static int set_wallpaper(GSettings *settings, sqlite3 *db, int brightness);

/* Copy of PATH */
char wallpaper_dir[PATH_MAX];

/* For storing the path of the current wallpaper */
char current_wallpaper[PATH_MAX];

/* Set up the arguments parser */
const char *argp_program_version = PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Program documentation */
static char doc[] = "nextwall -- a wallpaper rotator with some sense of time";

/* A description of the arguments we accept */
static char args_doc[] = "PATH";

/* The options we understand */
static struct argp_option options[] = {
    {"brightness", 'b', "N", 0, "Select wallpapers for night (0), twilight " \
        "(1), or day (2)"},
    {"interactive", 'i', 0, 0, "Run in interactive mode"},
    {"location", 'l', "LAT:LON", 0, "Specify latitude and longitude of your " \
        "current location"},
    {"recursion", 'r', 0, 0, "Causes --scan to look in subdirectories"},
    {"scan", 's', 0, 0, "Scan for images files in PATH. Also see the " \
        "--recursion option"},
    {"time", 't', 0, 0, "Find wallpapers that fit the time of day. Must be " \
        "used in combination with --location"},
    {"verbose", 'v', 0, 0, "Increase verbosity"},
    { 0 }
};

/* Used by main to communicate with parse_opt */
struct arguments {
    char *args[1]; /* PATH argument */
    char *location;
    int brightness, interactive, recursion, scan, time, verbose;
    double latitude, longitude;
};

/* Parse a single option */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    char tmp[80];
    char *lat, *lon;
    int rc, b;

    switch (key)
    {
        case 'b':
            if (!isdigit(*arg)) {
                fprintf(stderr, "Incorrect brightness value\n");
                argp_usage(state);
                break;
            }

            b = atoi(arg);
            if ( !(b == 0 || b == 1 || b == 2) ) {
                fprintf(stderr, "Incorrect brightness value\n");
                argp_usage(state);
                break;
            }

            arguments->brightness = b;
            break;
        case 'i':
            arguments->interactive = 1;
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

            rc = sscanf(lat, "%lf", &arguments->latitude);
            if (rc == 0) {
                fprintf(stderr, "Incorrect value for latitude\n");
                argp_usage(state);
            }
            rc = sscanf(lon, "%lf", &arguments->longitude);
            if (rc == 0) {
                fprintf(stderr, "Incorrect value for longitude\n");
                argp_usage(state);
            }
            break;
        case 'r':
            arguments->recursion = 1;
            break;
        case 's':
            arguments->scan = 1;
            break;
        case 't':
            arguments->time = 1;
            break;
        case 'v':
            arguments->verbose = verbose = 1;
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
            if (arguments->time && arguments->latitude == -1) {
                 fprintf(stderr, "Your location must be set with --location " \
                         "when using --time\n");
                 argp_usage(state);
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
    int found, i;
    sqlite3 *db;
    GSettings *settings;
    char *annfiles[3];
    char tmp[PATH_MAX];
    char *line = NULL;
    size_t linelen = 0;
    ssize_t read;

    // Create a new GSettings object
    settings = g_settings_new("org.gnome.desktop.background");

    // Default argument values
    arguments.brightness = -1;
    arguments.interactive = 0;
    arguments.latitude = -1;
    arguments.longitude = -1;
    arguments.recursion = 0;
    arguments.scan = 0;
    arguments.time = 0;
    arguments.verbose = 0;

    /* Parse arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    // Set the wallpaper directory
    strncpy(wallpaper_dir, arguments.args[0], sizeof wallpaper_dir);

    if ( stat(wallpaper_dir, &sts) != 0 || !S_ISDIR(sts.st_mode) ) {
        fprintf(stderr, "The wallpaper path %s doesn't exist.\n",
                wallpaper_dir);
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

    for (i = 0; i < 3; i++) {
        if (file_exists(annfiles[i])) {
            annfile = annfiles[i];
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
        if ( (rc = sqlite3_open(dbfile, &db)) != SQLITE_OK ) {
            fprintf(stderr, "Error: Can't open database: %s\n",
                    sqlite3_errmsg(db));
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
            local_brightness = get_local_brightness(arguments.latitude,
                    arguments.longitude);
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
                fprintf(stderr, "Error: Could not determine the local " \
                        "brightness value.\n");
                goto Return;
        }
    }

    // Set wallpaper_path
    if ( (nextwall(db, wallpaper_dir, local_brightness)) == -1 ) {
        fprintf(stderr, "No wallpapers found for directory %s. Try the " \
                "--scan option or remove the --time option.\n", wallpaper_dir);
        goto Return;
    }

    if (arguments.interactive) {
        fprintf(stderr, "Nextwall %s\n" \
                "Copyright (C) 2010-2013 Serrano Pereira\n" \
                "License GPLv3+: GNU GPL version 3 or later " \
                "<http://gnu.org/licenses/gpl.html>\n" \
                "Type 'help' for more information.\n" \
                "nextwall> ", PACKAGE_VERSION);
        while ( (read = getline(&line, &linelen, stdin)) != -1 ) {
            if (strcmp(line, "d\n") == 0) {
                get_background_uri(settings, current_wallpaper);
                fprintf(stderr, "Permanently remove %s from disk? (y/N) ",
                        current_wallpaper);
                read = getline(&line, &linelen, stdin);
                if ( read != -1 && strcmp(line, "y\n") == 0 && \
                        remove_wallpaper(db, current_wallpaper) == 0) {
                    fprintf(stderr, "Wallpaper removed\n");
                    if ( set_wallpaper(settings, db, local_brightness) == -1)
                        goto Return;
                }
            }
            else if (strcmp(line, "\n") == 0 || strcmp(line, "n\n") == 0) {
                if (set_wallpaper(settings, db, local_brightness) == -1)
                    goto Return;
            }
            else if (strcmp(line, "help\n") == 0) {
                fprintf(stderr,
                    "Nextwall is now running in interactive mode. The " \
                    "following commands are available:\n" \
                    "'d'\tPermanently remove the current wallpaper from disk\n" \
                    "'n'\tNext wallpaper (default)\n" \
                    "'q'\tExit nextwall\n");
            }
            else if (strcmp(line, "q\n") == 0) {
                goto Return;
            }
            else {
                fprintf(stderr, "Unknown command. Type 'help' to see the " \
                        "available commands.\n");
            }
            fprintf(stderr, "nextwall> ");
        }
    }
    else {
        set_wallpaper(settings, db, local_brightness);
    }

    goto Return;

Return:
    free(line);
    g_object_unref(settings);
    sqlite3_close(db);
    return 0;
}

int set_wallpaper(GSettings *settings, sqlite3 *db, int brightness) {
    int i;

    // Get the path of the current wallpaper
    get_background_uri(settings, current_wallpaper);

    // Make sure we select a different wallpaper
    for (i = 0; strcmp(wallpaper_path, current_wallpaper) == 0; i++) {
        if (i == 5) {
            fprintf(stderr, "Not enough wallpapers found. Select a different " \
                    "directory or use the --scan option.\n");
            return -1;
        }
        nextwall(db, wallpaper_dir, brightness);
    }

    // Set the new wallpaper
    eprintf("Setting wallpaper to %s\n", wallpaper_path);
    set_background_uri(settings, wallpaper_path);

    return 0;
}

