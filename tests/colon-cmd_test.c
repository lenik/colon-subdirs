/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Integration-style checks via --colon-dry-run on built binaries are done
 * separately; this file covers help/version wiring through colon_cmd_main.
 */

#include "colon-cmd.h"

#include <check.h>
#include <stdlib.h>

START_TEST(test_help_exits_zero) {
    char *argv[] = {"colon-mv", "--help", NULL};
    static const ColonCmdDesc desc = {
        .prog = "colon-mv",
        .colon = ":mv",
        .real_cmd = "mv",
        .style = COLON_STYLE_SRCDEST,
        .short_arg_opts = "St",
        .opt_args = NULL,
        .usage_extra = NULL,
        .options_help = NULL,
    };
    ck_assert_int_eq(colon_cmd_main(&desc, 2, argv), 0);
}
END_TEST

Suite *suite(void) {
    Suite *s = suite_create("colon-cmd");
    TCase *tc = tcase_create("help");
    tcase_add_test(tc, test_help_exits_zero);
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
