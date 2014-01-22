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

#define _GNU_SOURCE     /* for asprintf(), tm_gmtoff and tm_zone */

#include <config.h>
#include <argp.h>
#include <floatfann.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>  /* for open() */
#include <sys/stat.h>   /* for open() */
#include <fcntl.h>      /* for open() */

#include "cfgpath.h"
#include "nextwall.h"
#include "database.h"
#include "options.h"
#include "gnome.h"
#include "sunriset.h"
#include "std.h"

/* Define the global variable for verbosity */
int verbose = 0;

/* Main function */
int main(int argc, char **argv) {
    int rc = -1, local_brightness = -1;
    int exit_status = EXIT_SUCCESS;
    unsigned seed;
    char *annfile = NULL;
    char *line = NULL;
    char cfgpath[PATH_MAX];
    char current_wallpaper[PATH_MAX] = "\0";
    char dbfile[PATH_MAX];
    char wallpaper_path[PATH_MAX] = "\0";
    size_t linelen = 0;
    struct arguments arguments;
    struct fann *ann = NULL;
    GSettings *settings = NULL;
    sqlite3 *db = NULL;

    /* Default argument values */
    arguments.brightness = -1;
    arguments.interactive = 0;
    arguments.latitude = -1;
    arguments.longitude = -1;
    arguments.recursion = 0;
    arguments.scan = 0;
    arguments.time = 0;
    arguments.verbose = 0;

    /* Parse arguments; every option seen by parse_opt will be reflected
       in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    /* Define the wallpaper state */
    struct wallpaper_state wallpaper = {
        arguments.args[0],
        current_wallpaper,
        wallpaper_path
    };

    if ( !g_file_test(wallpaper.dir, G_FILE_TEST_IS_DIR) ) {
        fprintf(stderr, "Cannot access directory %s\n", wallpaper.dir);

        exit_status = EXIT_FAILURE;
        goto Return;
    }

    /* Set the user specific data storage folder. The folder is automatically
       created if it doesn't already exist. */
    get_user_data_folder(cfgpath, sizeof cfgpath, "nextwall");

    if (cfgpath[0] == 0) {
        fprintf(stderr, "Error: Unable to set the data storage folder.\n");

        exit_status = EXIT_FAILURE;
        goto Return;
    }

    /* Set the database file path */
    strcpy(dbfile, cfgpath);
    strcat(dbfile, "nextwall.db");

    /* Find the location of the ANN file */
    if (arguments.scan) {
        int i, ann_found;
        char *annfiles[3];

        if (asprintf(&annfile, "%snextwall.net", cfgpath) == -1) {
            fprintf(stderr, "Error: asprintf() failed\n");

            exit_status = EXIT_FAILURE;
            goto Return;
        }

        annfiles[0] = annfile;
        annfiles[1] = "/usr/local/share/nextwall/nextwall.net";
        annfiles[2] = "/usr/share/nextwall/nextwall.net";

        for (i = 0; i < 3; i++) {
            if ( (ann_found = g_file_test(annfiles[i], G_FILE_TEST_IS_REGULAR)) ) {
                eprintf("Using ANN %s\n", annfiles[i]);

                /* Initialize the ANN */
                ann = fann_create_from_file(annfiles[i]);

                break;
            }
        }

        if (!ann_found) {
            fprintf(stderr, "Error: Could not find ANN file nextwall.net\n");

            exit_status = EXIT_FAILURE;
            goto Return;
        }
    }

    /* Create the database if it doesn't exist */
    if ( !g_file_test(dbfile, G_FILE_TEST_IS_REGULAR) ) {
        eprintf("Creating database... ");
        if ( (rc = sqlite3_open(dbfile, &db)) == SQLITE_OK && \
                create_database(db) == 0 ) {
            eprintf("Done\n");
        }
        else {
            eprintf("Failed\n");
            fprintf(stderr, "Error: Creating database failed.\n");

            exit_status = EXIT_FAILURE;
            goto Return;
        }
    }

    /* Open database connection */
    if ( rc != SQLITE_OK ) {
        if ( sqlite3_open(dbfile, &db) != SQLITE_OK ) {
            fprintf(stderr, "Error: Can't open database: %s\n",
                    sqlite3_errmsg(db));

            exit_status = EXIT_FAILURE;
            goto Return;
        }
    }

    /* Search directory for wallpapers */
    if (arguments.scan) {
        int found;

        fprintf(stderr, "Scanning for new wallpapers...");
        found = scan_dir(db, wallpaper.dir, ann, arguments.recursion);
        fann_destroy(ann);
        fprintf(stderr, " Done\n");
        fprintf(stderr, "Found %d new wallpapers\n", found);
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

                exit_status = EXIT_FAILURE;
                goto Return;
        }
    }

    /* Set the seed for the random number generator */
    if ( read(open("/dev/urandom", O_RDONLY), &seed, sizeof seed) == -1 ) {
        exit_status = EXIT_FAILURE;
        goto Return;
    }
    srand(seed);

    /* Set the wallpaper path */
    if ( (nextwall(db, wallpaper.dir, local_brightness, wallpaper.path)) == -1 ) {
        fprintf(stderr, "No wallpapers found for directory %s. Try the " \
                "--scan option or remove the --time option.\n", wallpaper.dir);
        goto Return;
    }

    /* Create a GSettings object for the desktop background */
    settings = g_settings_new("org.gnome.desktop.background");

    if (arguments.interactive) {
        fprintf(stderr, "Nextwall %s\n" \
                "License: GNU GPL version 3 or later " \
                "<http://gnu.org/licenses/gpl.html>\n" \
                "Type 'help' for more information.\n" \
                "nextwall> ", PACKAGE_VERSION);
        while ( getline(&line, &linelen, stdin) != -1 ) {
            if (strcmp(line, "d\n") == 0) {
                get_background_uri(settings, wallpaper.current);
                fprintf(stderr, "Move wallpaper %s to trash? (y/N) ",
                        wallpaper.current);
                if ( getline(&line, &linelen, stdin) != -1 && \
                        strcmp(line, "y\n") == 0 && \
                        remove_wallpaper(db, wallpaper.current) == 0) {
                    fprintf(stderr, "Wallpaper removed\n");
                    if ( set_wallpaper(settings, db, local_brightness, &wallpaper) == -1)
                        goto Return;
                }
            }
            else if (strcmp(line, "\n") == 0 || strcmp(line, "n\n") == 0) {
                if (set_wallpaper(settings, db, local_brightness, &wallpaper) == -1)
                    goto Return;
            }
            else if (strcmp(line, "o\n") == 0) {
                get_background_uri(settings, wallpaper.current);
                open_image(wallpaper.current);
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
        set_wallpaper(settings, db, local_brightness, &wallpaper);
    }

    goto Return;

Return:
    if (annfile)
        free(annfile);
    if (line)
        free(line);
    if (settings)
        g_object_unref(settings);
    if (db)
        sqlite3_close(db);
    return exit_status;
}

/* Wrapper function for setting the wallpaper */
int set_wallpaper(GSettings *settings, sqlite3 *db, int brightness,
                  struct wallpaper_state *wallpaper) {
    int i, exists;

    /* Get the path of the current wallpaper */
    get_background_uri(settings, wallpaper->current);

    /* Make sure we select a different wallpaper and that the file exists */
    for (i = 0; !(exists = g_file_test(wallpaper->path, G_FILE_TEST_IS_REGULAR)) ||
            strcmp(wallpaper->path, wallpaper->current) == 0; i++) {
        if (i == 5) {
            fprintf(stderr, "Not enough wallpapers found. Select a different " \
                    "directory or use the --scan option.\n");
            return -1;
        }

        if (!exists) {
            eprintf("Wallpaper '%s' no longer exists. Deleting.\n",
                    wallpaper->path);
            remove_wallpaper(db, wallpaper->path);
        }

        nextwall(db, wallpaper->dir, brightness, wallpaper->path);
    }

    /* Set the new wallpaper */
    eprintf("Setting wallpaper to %s\n", wallpaper->path);
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
    char sr_s[6], ss_s[6], civ_start_s[6], civ_end_s[6];

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
            eprintf("Sun rises %s, sets %s %s\n", hours_to_hm(sunrise, sr_s),
                    hours_to_hm(sunset, ss_s), ltime.tm_zone);
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
                    hours_to_hm(civ_start, civ_start_s),
                    hours_to_hm(civ_end, civ_end_s), ltime.tm_zone);
            break;
        case +1:
            eprintf("Never darker than civil twilight\n");
            break;
        case -1:
            eprintf("Never as bright as civil twilight\n");
            break;
    }

    if (rs == 0 && civ == 0) {
        if (sunrise < htime && htime < sunset)
            return 2;
        else if (htime < civ_start || htime > civ_end)
            return 0;
        else
            return 1;
    }
    else {
        return -1;
    }
}
