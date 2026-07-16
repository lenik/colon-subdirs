/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"

static const ColonOptArg opt_args[] = {
    {'p', NULL},
    {'I', "ignore-fail-on-non-empty"},
    {0, "ignore-fail-on-non-empty"},
    {0, NULL},
};

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-rmdir",
        .colon = ":rmdir",
        .real_cmd = "rmdir",
        .style = COLON_STYLE_RMDIR,
        .rewrite = rewrite_rmdir,
        .short_arg_opts = "",
        .opt_args = opt_args,
        .usage_extra =
            "  :rmdir a/b:d/e/f  →  rmdir a/b/d/e/f, then empty a/b/d/e and a/b/d\n"
            "                      (empty directories only; BASE a/b is kept)",
    };
    return colon_cmd_main(&desc, argc, argv);
}
