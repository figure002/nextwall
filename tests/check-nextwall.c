/*
   Copyright 2013, Serrano Pereira <serrano.pereira@gmail.com>

   Copying and distribution of this file, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.
 */

/**
   Implements basic unit testing for Nextwall.

   This program uses Check, a unit testing framework for C.
 */

#include <stdlib.h>
#include <check.h>
#include <floatfann.h>

#include "std.h"

/* Location of the ANN file */
#define ANN_FILE "../data/ann/nextwall.net"

struct fann *ann = NULL;

void setup(void) {
    ann = fann_create_from_file(ANN_FILE);
}

void teardown(void) {
    fann_destroy(ann);
}

START_TEST(test_floatcmp)
{
    float a;
    float b;

    a = 0.19;
    b = 0.2;
    ck_assert( floatcmp(&a, &b) == -1 );

    b = -0.34;
    ck_assert( floatcmp(&a, &b) == 1 );

    b = a;
    ck_assert( floatcmp(&a, &b) == 0 );
}
END_TEST

START_TEST(test_ann)
{
    ck_assert( get_brightness(ann, 17.933842, 0.117967) == 0 );
    ck_assert( get_brightness(ann, 0.408988, 0.241489) == 1 );
    ck_assert( get_brightness(ann, 8.668308, 0.742252) == 2 );
    ck_assert( get_brightness(ann, 166.338012, 0.016205) == 0 );
    ck_assert( get_brightness(ann, 0.135857, 0.245151) == 1 );
    ck_assert( get_brightness(ann, -0.159653, 0.557191) == 2 );
}
END_TEST

Suite *nextwall_suite(void)
{
    Suite *s = suite_create("Nextwall");

    /* Core test case */
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_floatcmp);
    tcase_add_test(tc_core, test_ann);
    suite_add_tcase(s, tc_core);

    return s;
}

int main (void)
{
    int number_failed;
    Suite *s = nextwall_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

