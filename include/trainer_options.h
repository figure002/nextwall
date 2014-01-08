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

#ifndef NEXTWALL_TRAINER_OPTIONS_H
#define NEXTWALL_TRAINER_OPTIONS_H

#include <argp.h>

#include "config.h"

/* Set up the arguments parser */
const char *argp_program_version = PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Program documentation */
static char doc[] = "train_ann -- ANN trainer for nextwall";

/* A description of the arguments we accept */
static char args_doc[] = "PATH";

/* The options we understand */
static struct argp_option options[] = {
    {"reuse", 'r', 0, 0, "Use existing training data file to create the ANN"},
    {"pairs", 'p', "N", 0, "Number of training pairs to generate"},
    {"layers", 'l', "N", 0, "Number of neuron layers (default: 3)"},
    {"neurons", 'n', "N", 0, "Number of hidden neurons (default: 8)"},
    {"epochs", 't', "N", 0, "Maximum number of epochs (default: 500000)"},
    {"error", 'e', "N", 0, "Desired error (default: 0.001)"},
    {"output", 'o', "NAME", 0, "Base name of output files (default: nextwall)"},
    { 0 }
};

/* Used by main to communicate with parse_opt */
struct arguments {
    char *args[1]; /* PATH argument */
    char *output;
    int layers, neurons, epochs, pairs, reuse;
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
            arguments->pairs = atoi(arg);
            break;
        case 't':
            arguments->epochs = atoi(arg);
            break;
        case 'e':
            arguments->error = atof(arg);
            break;
        case 'o':
            arguments->output = arg;
            break;
        case 'r':
            arguments->reuse = 1;
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num >= 1) {
                 // Too many arguments
                 argp_usage(state);
            }
            arguments->args[state->arg_num] = arg;
            break;

        case ARGP_KEY_END:
            if (state->arg_num < 1 && arguments->reuse == 0) {
                // Too few arguments
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

#endif

