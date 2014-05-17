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

/**
   Trainer for the Artificial Neural Network (ANN).

   Collects brightness values for images from user input and writes this data
   to a training data file (*.dat). The training data is then used to build the
   ANN (*.net).
 */

#include <argp.h>
#include <dirent.h>
#include <fann.h>
#include <gio/gio.h>
#include <limits.h>
#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "config.h"
#include "gnome.h"
#include "image.h"
#include "std.h"
#include "trainer-options.h"

/* Number of input values for the ANN */
#define NUM_INPUT 2

/* Number of output values for the ANN */
#define NUM_OUTPUT 3

/* Function prototypes */
int set_training_pairs(FILE *fp, const char *base, int max_pairs);


int main(int argc, char **argv) {
    struct arguments arguments;
    char datafile[PATH_MAX], netfile[PATH_MAX];
    const unsigned int epochs_between_reports = 1000;

    // Default argument values
    arguments.error = 0.00001;
    arguments.epochs = 10000;
    arguments.pairs = -1;
    arguments.layers = 3;
    arguments.neurons = 8;
    arguments.output = "nextwall";
    arguments.reuse = 0;

    /* Parse arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    // Set the file paths
    sprintf(datafile, "%s.dat", arguments.output);
    sprintf(netfile, "%s.net", arguments.output);

    if (!arguments.reuse) {
        FILE *fp;
        int n_pairs;

        if (arguments.pairs == -1) {
            fprintf(stderr, "Error: Number of training pairs is not set, " \
                    "use --pairs\n");
            return -1;
        }

        // Open data file
        if ( (fp = fopen(datafile, "w")) == NULL ) {
            fprintf(stderr, "Failed to open %s for writing\n", datafile);
            exit(0);
        }

        // Write training data to file
        fprintf(fp, "%d %d %d\n", arguments.pairs, NUM_INPUT, NUM_OUTPUT);
        n_pairs = set_training_pairs(fp, arguments.args[0], arguments.pairs);
        fclose(fp);

        if (n_pairs == -1) {
            return -1;
        }
        if (arguments.pairs != n_pairs) {
            fprintf(stderr, "Error: Didn't find enough images. Set --pairs " \
                    "to a lower value.\n");
            return -1;
        }
    }

    // Create the ANN
    fprintf(stderr, "Creating the Artificial Neural Network with training " \
            "data from %s...\n", datafile);
    struct fann *ann = fann_create_standard(arguments.layers, NUM_INPUT,
        arguments.neurons, NUM_OUTPUT);

    fann_set_activation_function_hidden(ann, FANN_SIGMOID_SYMMETRIC);
    fann_set_activation_function_output(ann, FANN_SIGMOID_SYMMETRIC);

    fann_train_on_file(ann, datafile, arguments.epochs,
        epochs_between_reports, arguments.error);

    fann_save(ann, netfile);

    fann_destroy(ann);

    return 0;
}

/**
  Write training pairs to a training data file.

  It scans for image files in a directory and lets the user define the
  brightness for each image.

  @param[in] fp File pointer.
  @param[in] base The image directory.
  @param[in] max_pairs The maximum mumber of training pairs that will be
             written to file.
  @return The number training pairs that were written to file.
 */
int set_training_pairs(FILE *fp, const char *base, int max_pairs) {
    DIR *dir;
    struct dirent *entry;
    char tmp[PATH_MAX];
    char path[PATH_MAX];
    double kurtosis, lightness;
    int n_pairs = 0;
    char *line = NULL;
    size_t len = 0;
    GSettings *settings;

    // Create a new GSettings object
    settings = g_settings_new("org.gnome.desktop.background");

    // Initialize Magic Number Recognition Library
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    magic_load(magic, NULL);

    if (!(dir = opendir(base)))
        return -1;
    if (!(entry = readdir(dir)))
        return -1;

    fprintf(stderr, "Defining the image brightness for %d images. Each " \
            "image will be displayed on your desktop.\n", max_pairs);
    fprintf(stderr, "At any time you can enter 's' to skip an image, or " \
            "'q' to quit.\n\n");

    do {
        snprintf(tmp, sizeof tmp, "%s/%s", base, entry->d_name);
        if ( realpath(tmp, path) == NULL )
            goto Return;

        if (entry->d_type == DT_DIR) {
            continue;
        }
        else if (strstr(magic_file(magic, path), "image")) {
            // Get image kurtosis and lightness.
            if (get_image_info(path, &kurtosis, &lightness) == -1)
                continue;

            set_background_uri(settings, path);
            fprintf(stderr, "Image: %s\n", path);
            fprintf(stderr, "Is this image dark (0), intermediate (1), or " \
                    "light (2)? ");

            // Get image brightness value from user input.
            while ( getline(&line, &len, stdin) != -1 ) {
                if ( strcmp(line, "0\n") == 0 ) {
                    fprintf(fp, "%f %f\n", kurtosis, lightness);
                    fprintf(fp, "1 0 0\n");
                    fprintf(stderr, "Marked dark\n\n");
                    n_pairs++;
                    break;
                }
                else if ( strcmp(line, "1\n") == 0 ) {
                    fprintf(fp, "%f %f\n", kurtosis, lightness);
                    fprintf(fp, "0 1 0\n");
                    fprintf(stderr, "Marked intermediate\n\n");
                    n_pairs++;
                    break;
                }
                else if ( strcmp(line, "2\n") == 0 ) {
                    fprintf(fp, "%f %f\n", kurtosis, lightness);
                    fprintf(fp, "0 0 1\n");
                    fprintf(stderr, "Marked light\n\n");
                    n_pairs++;
                    break;
                }
                else if ( strcmp(line, "s\n") == 0 ) {
                    fprintf(stderr, "Skipped\n\n");
                    break;
                }
                else if ( strcmp(line, "q\n") == 0 ) {
                    fprintf(stderr, "Abort\n\n");
                    n_pairs = -1;
                    goto Return;
                }
                else
                    fprintf(stderr, "Incorrect input. Try again: ");
            }
        }
    } while ( (entry = readdir(dir)) && n_pairs < max_pairs );

    goto Return;

Return:
    free(line);
    g_object_unref(settings);
    magic_close(magic);
    closedir(dir);
    return n_pairs;
}

