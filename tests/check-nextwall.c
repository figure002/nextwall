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

#include "std.h"

void setup(void) {
    ;
}

void teardown(void) {
    ;
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

Suite *nextwall_suite(void)
{
    Suite *s = suite_create("Nextwall");

    /* Core test case */
    TCase *tc_core = tcase_create("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_floatcmp);
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

