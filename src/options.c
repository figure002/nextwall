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
#include <stdlib.h>
#include <string.h>

#include "nextwall.h"
#include "config.h"
#include "options.h"

/* Set up the arguments parser */
const char *argp_program_version = PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Program documentation */
//static char doc[] = "nextwall - A wallpaper rotator with some sense of time.";
static char doc[] = "\nOptions:";

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

/* Parse a single option */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    char tmp[80];
    char *lat, *lon;
    int b;

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
            arguments->time = 1;
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

            if ((arguments->latitude = strtod(lat, NULL)) == 0) {
                fprintf(stderr, "Incorrect value for latitude\n");
                argp_usage(state);
            }
            if ((arguments->longitude = strtod(lon, NULL)) == 0) {
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
                 // Too many arguments
                 argp_usage(state);
            }
            arguments->args[state->arg_num] = arg;
            break;

        case ARGP_KEY_END:
            if (state->arg_num == 0) {
                // Use the default wallpaper directory if none specified
                arguments->args[0] = DEFAULT_WALLPAPER_DIR;
            }
            if (arguments->brightness == -1 && arguments->time && \
                    arguments->latitude == -1) {
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
struct argp argp = { options, parse_opt, args_doc, doc };

