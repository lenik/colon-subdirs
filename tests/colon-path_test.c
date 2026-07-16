/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-path.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

START_TEST(test_colon_basic) {
    ColonPath p;
    ck_assert_int_eq(colon_path_parse(&p, "a/b:c/d"), 0);
    ck_assert_int_eq(p.is_colon, 1);
    ck_assert_str_eq(p.base, "a/b");
    ck_assert_str_eq(p.leaf, "c/d");
    ck_assert_str_eq(p.physical, "a/b/c/d");
    colon_path_free(&p);
}
END_TEST

START_TEST(test_colon_leading_slash_leaf) {
    ColonPath p;
    ck_assert_int_eq(colon_path_parse(&p, "a/b:/c/d/e"), 0);
    ck_assert_str_eq(p.leaf, "c/d/e");
    ck_assert_str_eq(p.physical, "a/b/c/d/e");
    colon_path_free(&p);
}
END_TEST

START_TEST(test_host_path_not_colon) {
    ColonPath p;
    ck_assert_int_eq(colon_path_parse(&p, "host:path"), 0);
    ck_assert_int_eq(p.is_colon, 0);
    ck_assert_str_eq(p.physical, "host:path");
    colon_path_free(&p);
}
END_TEST

START_TEST(test_rsync_daemon_not_colon) {
    ColonPath p;
    ck_assert_int_eq(colon_path_parse(&p, "host::module"), 0);
    ck_assert_int_eq(p.is_colon, 0);
    colon_path_free(&p);
}
END_TEST

START_TEST(test_join_mkdir) {
    char *j = colon_join("../x", "c/d");
    ck_assert_ptr_nonnull(j);
    ck_assert_str_eq(j, "../x/c/d");
    free(j);

    char *d = colon_dirname("../x/c/d");
    ck_assert_str_eq(d, "../x/c");
    free(d);
}
END_TEST

Suite *suite(void) {
    Suite *s = suite_create("colon-path");
    TCase *tc = tcase_create("core");
    tcase_add_test(tc, test_colon_basic);
    tcase_add_test(tc, test_colon_leading_slash_leaf);
    tcase_add_test(tc, test_host_path_not_colon);
    tcase_add_test(tc, test_rsync_daemon_not_colon);
    tcase_add_test(tc, test_join_mkdir);
    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    Suite *s = suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n == 0 ? 0 : 1;
}
