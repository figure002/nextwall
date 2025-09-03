/*
  This file is part of nextwall - a wallpaper rotator with some sense of time.

   Copyright 2004, Davyd Madeley <davyd@madeley.id.au>
   Copyright 2010-2013, Serrano Pereira <serrano@bitosis.nl>

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

#define _GNU_SOURCE     /* asprintf tm_gmtoff tm_zone */

#include <argp.h>
#include <bsd/string.h> /* strlcpy strlcat */
#include <config.h>
#include <errno.h>
#include <fcntl.h>      /* open */
#include <floatfann.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <locale.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>     /* strerr strcmp */
#include <sys/stat.h>   /* open */
#include <sys/types.h>  /* open */
#include <time.h>       /* localtime_r */
#include <unistd.h>

#include "cfgpath.h"
#include "nextwall.h"
#include "database.h"
#include "options.h"
#include "gnome.h"
#include "sunriset.h"
#include "std.h"

extern int errno;

/* Define the global variable for verbosity */
int nextwall_verbose = 0;

int main(int argc, char **argv) {
    int rc = -1;
    int local_brightness = -1;
    int exit_status = EXIT_SUCCESS;
    unsigned seed;
    char *ann_path = NULL;
    char user_data_path[PATH_MAX];
    char current_wallpaper_path[PATH_MAX] = "\0";
    char db_path[PATH_MAX];
    char wallpaper_path[PATH_MAX] = "\0";
    struct arguments arguments;
    struct fann *ann = NULL;
    GSettings *settings = NULL;
    sqlite3 *db = NULL;

    /* Default argument values */
    arguments.brightness = -1;
    arguments.interactive = 0;
    arguments.latitude = -1;
    arguments.longitude = -1;
    arguments.print = false;
    arguments.recursion = 0;
    arguments.scan = 0;
    arguments.time = 0;
    arguments.verbose = 0;

    /* Set the locale to something that works with FANN configuration files. */
    setlocale(LC_ALL, "C");

    /* Parse arguments; every option seen by parse_opt will be reflected
       in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    /* Define the wallpaper state */
    struct wallpaper_state wallpaper = {
        arguments.args[0],
        current_wallpaper_path,
        wallpaper_path
    };

    if (!g_file_test(wallpaper.dir, G_FILE_TEST_IS_DIR)) {
        fprintf(stderr, "Cannot access directory %s\n", wallpaper.dir);
        goto Return_failure;
    }

    /* Set the user specific data storage folder. The folder is automatically
       created if it doesn't already exist. */
    get_user_data_folder(user_data_path, sizeof user_data_path, "nextwall");

    if (user_data_path[0] == 0) {
        fprintf(stderr, "Error: Unable to set the data storage folder.\n");
        goto Return_failure;
    }

    /* Set the database file path */
    if (strlcpy(db_path, user_data_path, sizeof db_path) >= sizeof db_path) {
        goto Too_long;
    }

    if (strlcat(db_path, "nextwall.db", sizeof db_path) >= sizeof db_path) {
        goto Too_long;
    }

    /* Find the location of the ANN file */
    if (arguments.scan) {
        int i, ann_found;
        char *ann_paths[3];

        if (asprintf(&ann_path, "%snextwall.net", user_data_path) == -1) {
            fprintf(stderr, "asprintf() failed: %s\n", strerror(errno));

            goto Return_failure;
        }

        ann_paths[0] = ann_path;
        ann_paths[1] = "/usr/local/share/nextwall/nextwall.net";
        ann_paths[2] = "/usr/share/nextwall/nextwall.net";

        for (i = 0; i < 3; i++) {
            if ( (ann_found = g_file_test(ann_paths[i], G_FILE_TEST_IS_REGULAR)) ) {
                eprintf("Using ANN %s\n", ann_paths[i]);

                /* Initialize the ANN */
                ann = fann_create_from_file(ann_paths[i]);

                break;
            }
        }

        if (!ann_found) {
            fprintf(stderr, "Error: Could not find ANN file nextwall.net\n");

            goto Return_failure;
        }
    }

    /* Create the database if it doesn't exist */
    if ( !g_file_test(db_path, G_FILE_TEST_IS_REGULAR) ) {
        eprintf("Creating database... ");
        if ( (rc = sqlite3_open(db_path, &db)) == SQLITE_OK && \
                create_database(db) == 0 ) {
            eprintf("Done\n");
        }
        else {
            eprintf("Failed\n");
            fprintf(stderr, "Error: Creating database failed.\n");

            goto Return_failure;
        }
    }

    /* Open database connection */
    if ( rc != SQLITE_OK ) {
        if ( sqlite3_open(db_path, &db) != SQLITE_OK ) {
            fprintf(stderr, "Error: Can't open database: %s\n",
                    sqlite3_errmsg(db));

            goto Return_failure;
        }
    }

    /* Search directory for wallpapers */
    if (arguments.scan) {
        int found;

        fprintf(stderr, "Scanning for new wallpapers...\n");
        found = scan_dir(db, wallpaper.dir, ann, arguments.recursion);
        fann_destroy(ann);
        fprintf(stderr, "\nFound %d new wallpapers\n", found);
        goto Return;
    }

    /* Get local brightness */
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

                goto Return_failure;
        }
    }

    /* Set the seed for the random number generator */
    if ( read(open("/dev/urandom", O_RDONLY), &seed, sizeof seed) == -1 ) {
        goto Return_failure;
    }

    srand(seed);

    /* Set the wallpaper path */
    if ( (nextwall(db, wallpaper.dir, local_brightness, wallpaper.path)) == -1 ) {
        fprintf(stderr,
                "No wallpapers found for directory %s. Try the " \
                "--scan option or remove the --time option.\n",
                wallpaper.dir);
        goto Return;
    }

    /* Create a GSettings object for the desktop background */
    settings = g_settings_new("org.gnome.desktop.background");

    if (arguments.interactive) {
        char *input;
        char shell_prompt[100];
        int rc;

        fprintf(stderr,
                "Nextwall %s\n" \
                "License: GNU GPL version 3 or later " \
                "<http://gnu.org/licenses/gpl.html>\n" \
                "Type 'help' for more information.\n",
                PACKAGE_VERSION);

        // Configure readline to auto-complete paths when the tab key is hit.
        rl_bind_key('\t', rl_complete);

        // Set the prompt text.
        snprintf(shell_prompt, sizeof(shell_prompt), "nextwall> ");

        for(;;) {
            // Display prompt and read input.
            input = readline(shell_prompt);

            // Check for EOF.
            if (!input) {
                fprintf(stderr, "Bye\n");
                break;
            }

            // If the line has any text in it, save it on the history.
            if (input && *input) {
                add_history(input);
            }

            // Check if the directory still exists.
            if ( !g_file_test(wallpaper.dir, G_FILE_TEST_IS_DIR) ) {
                fprintf(stderr, "Cannot access directory %s\n", wallpaper.dir);

                goto Return_failure;
            }

            if (strcmp(input, "d") == 0) {
                if (get_background_uri(settings, wallpaper.current) != 0) {
                    fprintf(stderr, "Error: failed to get the current wallpaper\n");
                    goto Return_failure;
                }

                fprintf(stderr, "Move wallpaper %s to trash? (y/N) ",
                        wallpaper.current);

                if ((input = readline("")) && strcmp(input, "y") == 0) {
                    rc = remove_wallpaper(db, wallpaper.current, true);

                    if (rc == 0 && set_wallpaper(settings, db, local_brightness, &wallpaper, false) == -1) {
                        goto Return;
                    }
                }
            }
            else if (strcmp(input, "") == 0 || strcmp(input, "n") == 0) {
                if (set_wallpaper(settings, db, local_brightness, &wallpaper, false) == -1) {
                    goto Return;
                }
            }
            else if (strcmp(input, "o") == 0) {
                if (get_background_uri(settings, wallpaper.current) != 0) {
                    fprintf(stderr, "Error: failed to get the current wallpaper\n");
                    goto Return_failure;
                }

                open_image(wallpaper.current);
            }
            else if (strcmp(input, "help") == 0) {
                fprintf(stderr,
                    "Nextwall is now running in interactive mode. The " \
                    "following commands are available:\n" \
                    "'d'\tDelete the current wallpaper\n" \
                    "'n'\tNext wallpaper (default)\n" \
                    "'o'\tOpen the current wallpaper\n" \
                    "'q'\tExit nextwall\n");
            }
            else if (strcmp(input, "q") == 0) {
                goto Return;
            }
            else {
                fprintf(stderr,
                        "Unknown command. Type 'help' to see the " \
                        "available commands.\n");
            }

            free(input);
        }
    }
    else {
        set_wallpaper(settings, db, local_brightness, &wallpaper, arguments.print);
    }

    goto Return;

Too_long:
    fprintf(stderr, "Error: string too long\n");
    goto Return_failure;

Return_failure:
    exit_status = EXIT_FAILURE;
    goto Return;

Return:
    if (ann_path) {
        free(ann_path);
    }
    if (settings) {
        g_object_unref(settings);
    }
    if (db) {
        sqlite3_close(db);
    }
    return exit_status;
}

/* Wrapper function for setting the wallpaper */
int set_wallpaper(GSettings *settings,
                  sqlite3 *db,
                  int brightness,
                  struct wallpaper_state *wallpaper,
                  bool print_only) {
    int i;
    int file_exists;

    /* Get the path of the current wallpaper */
    if (get_background_uri(settings, wallpaper->current) != 0) {
        fprintf(stderr, "Error: failed to get the current wallpaper\n");
        return -1;
    }

    /* Make sure we select a different wallpaper and that the file exists */
    for (i = 0; !(file_exists = g_file_test(wallpaper->path, G_FILE_TEST_IS_REGULAR)) ||
                strcmp(wallpaper->path, wallpaper->current) == 0; i++) {
        if (i == 5) {
            // Give up after 5 tries.
            fprintf(stderr,
                    "Not enough wallpapers found. Select a different " \
                    "directory or use the --scan option.\n");
            return -1;
        }

        if (!file_exists) {
            eprintf("Wallpaper '%s' no longer exists. Removing from database.\n",
                    wallpaper->path);
            remove_wallpaper(db, wallpaper->path, false);

            /* Don't increment if the file was moved/deleted. */
            --i;
        }

        nextwall(db, wallpaper->dir, brightness, wallpaper->path);
    }

    if (print_only) {
        /* Print wallpaper and exit */
        fprintf(stdout, "%s", wallpaper->path);
        return 0;
    }

    eprintf("Setting wallpaper to %s\n", wallpaper->path);

    /* Set the new wallpaper */
    if (set_background_uri(settings, wallpaper->path) == -1) {
        fprintf(stderr, "Error: failed to set the background.\n");
    }

    return 0;
}

/**
  Return the local brightness value.

  This function uses sunriset.h to calculate sunrise, sunset, and civil
  twilight times.

  @param[in] lat The latitude of the current location.
  @param[in] lon The longitude of the current location.
  @return Returns 0 for night, 1 for twilight, or 2 for day, depending on
          local time. Returns -1 if brightness could not be determined.
 */
int get_local_brightness(double lat, double lon) {
    struct tm ltime;
    time_t now;
    double htime, sunrise, sunset, civ_start, civ_end;
    int year, month, day, gmt_offset_h, rs, civ;
    char sunrise_str[6], sunset_str[6], civ_start_str[6], civ_end_str[6];

    time(&now);
    localtime_r(&now, &ltime);
    year = ltime.tm_year + 1900;
    month = ltime.tm_mon + 1;
    day = ltime.tm_mday;

    /* GMT offset in hours with local time zone */
    gmt_offset_h = ltime.tm_gmtoff / 3600;

    /* Current time in hours */
    htime = (double)ltime.tm_hour + (double)ltime.tm_min / 60.0;

    /* Set local sunrise, sunset, and civil twilight times */
    rs = sun_rise_set(year, month, day, lon, lat, &sunrise, &sunset);
    civ  = civil_twilight(year, month, day, lon, lat, &civ_start, &civ_end);

    switch (rs) {
        case 0:
            sunrise += gmt_offset_h;
            sunset += gmt_offset_h;
            eprintf("Sun rises %s, sets %s %s\n",
                    hours_to_hm(sunrise, sunrise_str),
                    hours_to_hm(sunset, sunset_str),
                    ltime.tm_zone);
            break;

        case +1:
            eprintf("Sun above horizon\n");
            return 2;
            break;

        case -1:
            eprintf("Sun below horizon\n");
            return 0;
            break;
    }

    switch (civ) {
        case 0:
            civ_start += gmt_offset_h;
            civ_end += gmt_offset_h;
            eprintf("Civil twilight starts %s, ends %s %s\n",
                    hours_to_hm(civ_start, civ_start_str),
                    hours_to_hm(civ_end, civ_end_str),
                    ltime.tm_zone);
            break;

        case +1:
            eprintf("Never darker than civil twilight\n");
            break;

        case -1:
            eprintf("Never as bright as civil twilight\n");
            break;
    }

    if (rs == 0 && civ == 0) {
        if (sunrise < htime && htime < sunset) {
            return 2;
        }
        else if (htime < civ_start || htime > civ_end) {
            return 0;
        }
        else {
            return 1;
        }
    }
    else {
        return -1;
    }
}
