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

#include <argp.h>
#include <floatfann.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "cfgpath.h"
#include "nextwall.h"
#include "config.h"
#include "options.h"
#include "gnome.h"

/* Function prototypes */
int set_wallpaper(GSettings *settings, sqlite3 *db, int brightness);

/* Path to user configurations directory */
char cfgpath[PATH_MAX];

/* Copy of PATH */
char wallpaper_dir[PATH_MAX];

/* For storing the path of the current wallpaper */
char current_wallpaper[PATH_MAX];

/* Define the global verbose variable */
int verbose = 0;

/* Define the variable for the wallpaper path */
char wallpaper_path[PATH_MAX];

/* Main function */
int main(int argc, char **argv) {
    struct arguments arguments;
    int rc = -1, local_brightness = -1;
    int ann_found, found, i;
    sqlite3 *db;
    GSettings *settings;
    struct fann *ann = NULL;
    char *annfiles[3];
    char dbfile[PATH_MAX];
    char tmp[PATH_MAX];
    char *line;
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

    if ( !g_file_test(wallpaper_dir, G_FILE_TEST_IS_DIR) ) {
        fprintf(stderr, "Cannot access directory %s\n", wallpaper_dir);
        return 1;
    }

    /* Set the user specific data storage folder. The folder is
       automatically created if it doesn't already exist. */
    get_user_data_folder(cfgpath, sizeof cfgpath, "nextwall");

    if (cfgpath[0] == 0) {
        fprintf(stderr, "Error: Unable to set the data storage folder.\n");
        return 1;
    }

    // Set the database file path
    strcpy(dbfile, cfgpath);
    strcat(dbfile, "nextwall.db");

    // Set the ANN file path
    if (arguments.scan) {
        strcpy(tmp, cfgpath);
        strcat(tmp, "nextwall.net");
        annfiles[0] = tmp;
        annfiles[1] = "/usr/local/share/nextwall/nextwall.net";
        annfiles[2] = "/usr/share/nextwall/nextwall.net";

        for (i = 0; i < 3; i++) {
            if ( (ann_found = g_file_test(annfiles[i], G_FILE_TEST_IS_REGULAR)) ) {
                eprintf("Using ANN %s\n", annfiles[i]);

                // Initialize the ANN
                ann = fann_create_from_file(annfiles[i]);

                break;
            }
        }

        if (!ann_found) {
            fprintf(stderr, "Error: Could not find ANN file nextwall.net\n");
            return 1;
        }

    }

    // Create the database if it doesn't exist.
    if ( !g_file_test(dbfile, G_FILE_TEST_IS_REGULAR) ) {
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
        found = scan_dir(db, wallpaper_dir, ann, arguments.recursion);
        fann_destroy(ann);
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
                "License GPLv3+: GNU GPL version 3 or later " \
                "<http://gnu.org/licenses/gpl.html>\n" \
                "Type 'help' for more information.\n" \
                "nextwall> ", PACKAGE_VERSION);
        while ( (read = getline(&line, &linelen, stdin)) != -1 ) {
            if (strcmp(line, "d\n") == 0) {
                get_background_uri(settings, current_wallpaper);
                fprintf(stderr, "Move wallpaper %s to trash? (y/N) ",
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
            else if (strcmp(line, "o\n") == 0) {
                get_background_uri(settings, current_wallpaper);
                open_image(current_wallpaper);
            }
            else if (strcmp(line, "help\n") == 0) {
                fprintf(stderr,
                    "Nextwall is now running in interactive mode. The " \
                    "following commands are available:\n" \
                    "'d'\tDelete the current wallpaper\n" \
                    "'n'\tNext wallpaper (default)\n" \
                    "'o'\tOpen the current wallpaper\n" \
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

/* Wrapper function for setting the wallpaper */
int set_wallpaper(GSettings *settings, sqlite3 *db, int brightness) {
    int i, exists;

    // Get the path of the current wallpaper
    get_background_uri(settings, current_wallpaper);

    // Make sure we select a different wallpaper and that the file exists.
    for (i = 0; !(exists = g_file_test(wallpaper_path, G_FILE_TEST_IS_REGULAR)) ||
            strcmp(wallpaper_path, current_wallpaper) == 0; i++) {
        if (i == 5) {
            fprintf(stderr, "Not enough wallpapers found. Select a different " \
                    "directory or use the --scan option.\n");
            return -1;
        }

        if (!exists) {
            eprintf("Wallpaper '%s' no longer exists. Deleting.\n",
                    wallpaper_path);
            remove_wallpaper(db, wallpaper_path);
        }

        nextwall(db, wallpaper_dir, brightness);
    }

    // Set the new wallpaper
    eprintf("Setting wallpaper to %s\n", wallpaper_path);
    if (set_background_uri(settings, wallpaper_path) == -1) {
        fprintf(stderr, "Error: failed to set the background.\n");
    }

    return 0;
}

