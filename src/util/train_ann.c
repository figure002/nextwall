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

   Obtains predefined brightness values from a selectively populated nextwall
   database. It uses these values to write training data to a file (*.dat),
   and uses that training data to build the ANN (*.net).
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <argp.h>
#include <sqlite3.h>
#include <fann.h>

#include "config.h"

/* Number of input values for the ANN */
#define NUM_INPUT 2

/* Number of output values for the ANN */
#define NUM_OUTPUT 3


/* Function prototypes */
int file_exists(const char * filename);
int get_num_training_pairs(sqlite3 *db);
int ntp_callback(void *notused, int argc, char **argv, char **colnames);
int make_data_file(sqlite3 *db);
int mdf_callback(void *notused, int argc, char **argv, char **colnames);


/* Pointer to the data file */
FILE *fp;

/* The total number of training pairs */
int num_training_pairs = 0;

/* Set up the arguments parser */
const char *argp_program_version = PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Program documentation */
static char doc[] = "train_ann -- ANN trainer for nextwall";

/* A description of the arguments we accept */
static char args_doc[] = "DB_FILE";

/* The options we understand */
static struct argp_option options[] = {
    {"layers", 'l', "N", 0, "Number of neuron layers"},
    {"neurons", 'n', "N", 0, "Number of hidden neurons"},
    {"epochs", 'p', "N", 0, "Maximum number of epochs"},
    {"error", 'e', "N", 0, "Desired error"},
    {"output", 'o', "outfile", 0, "Base name of output files"},
    { 0 }
};

/* Used by main to communicate with parse_opt */
struct arguments {
    char *args[1]; /* DB_FILE argument */
    char *output;
    unsigned int layers, neurons, epochs;
    float error;
};

/* Parse a single option */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key)
    {
        case 'l':
            arguments->layers = atoi(arg);
            break;
        case 'n':
            arguments->neurons = atoi(arg);
            break;
        case 'p':
            arguments->epochs = atoi(arg);
            break;
        case 'e':
            arguments->error = atof(arg);
            break;
        case 'o':
            arguments->output = arg;
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num >= 1) {
                 /* Too many arguments */
                 argp_usage(state);
            }
            arguments->args[state->arg_num] = arg;
            break;

        case ARGP_KEY_END:
            if (state->arg_num < 1) {
                /* Too few arguments */
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
    char datafile[PATH_MAX], netfile[PATH_MAX];
    const unsigned int epochs_between_reports = 1000;
    sqlite3 *db;

    // Default argument values
    arguments.error = 0.001;
    arguments.epochs = 500000;
    arguments.layers = 3;
    arguments.neurons = 8;
    arguments.output = "nextwall";

    /* Parse arguments; every option seen by parse_opt will
       be reflected in arguments. */
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    // Set the file paths
    char *dbfile = arguments.args[0];
    sprintf(datafile, "%s.dat", arguments.output);
    sprintf(netfile, "%s.net", arguments.output);

    if ( !file_exists(dbfile) ) {
        printf("File not found: %s\n", dbfile);
        exit(1);
    }

    // Open data file
    if ( (fp = fopen(datafile, "w")) == NULL ) {
        printf("Failed to open %s for writing\n", datafile);
        exit(0);
    }

    // Open database connection
    if ( sqlite3_open(dbfile, &db) != 0 ) {
        fprintf(stderr, "Error: Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Write training data to file
    get_num_training_pairs(db);
    fprintf(fp, "%d %d %d\n", num_training_pairs, NUM_INPUT, NUM_OUTPUT);
    make_data_file(db);
    fclose(fp);
    sqlite3_close(db);

    // Create the ANN
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
  Check if a file exists and is readable.

  @param[in] filename The path to the file.
  @return 1 if the file exists, 0 otherwise.
 */
int file_exists(const char *filename) {
    FILE *f;
    if ( (f = fopen(filename, "r")) != NULL ) {
        fclose(f);
        return 1;
    }
    return 0;
}

/**
  Set the total number of training pairs.

  @param[in] db The SQLite database handler.
 */
int get_num_training_pairs(sqlite3 *db) {
    int rc;
    char sql[] = "SELECT COUNT(id) FROM wallpapers WHERE brightness IS NOT NULL;";

    rc = sqlite3_exec(db, sql, ntp_callback, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", sql);
        exit(1);
    }
    return 0;
}

/**
  Callback function for get_num_training_pairs().

  Sets `num_training_pairs` to the total number of training pairs.
 */
int ntp_callback(void *notused, int argc, char **argv, char **colnames) {
    num_training_pairs = atoi(argv[0]);
    return 0;
}

/**
  Create the training data file.

  @param[in] db The SQLite database handler.
 */
int make_data_file(sqlite3 *db) {
    int rc;
    char sql[] = "SELECT kurtosis, lightness, brightness FROM wallpapers WHERE brightness IS NOT NULL;";

    rc = sqlite3_exec(db, sql, mdf_callback, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error: Failed to execute query: %s\n", sql);
        exit(1);
    }
    return 0;
}

/**
  Callback function for make_data_file().

  Writes the training pairs to the file handler `fp`.
 */
int mdf_callback(void *notused, int argc, char **argv, char **colnames) {
    int b;

    fprintf(fp, "%s %s\n", argv[0], argv[1]);

    b = atoi(argv[2]);
    if (b == 0)
        fprintf(fp, "1 0 0\n");
    else if (b == 1)
        fprintf(fp, "0 1 0\n");
    else if (b == 2)
        fprintf(fp, "0 0 1\n");
    else {
        printf("Found incorrect value %d", b);
        fclose(fp);
        exit(1);
    }

    return 0;
}

