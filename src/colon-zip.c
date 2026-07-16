/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"

static const ColonOptArg opt_args[] = {
    {'b', NULL},
    {'n', NULL},
    {'t', NULL},
    {'P', NULL},
    {0, NULL},
};

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-zip",
        .colon = ":zip",
        .real_cmd = "zip",
        .style = COLON_STYLE_ZIP,
        /* zip option letters that take a separate argument (common set) */
        .short_arg_opts = "bntdP",
        .opt_args = opt_args,
        .usage_extra =
            "  :zip a.zip a/b:c/d/e\n"
            "      archive members are stored as c/d/e/... (not a/b/c/d/e/...)",
    };
    return colon_cmd_main(&desc, argc, argv);
}
