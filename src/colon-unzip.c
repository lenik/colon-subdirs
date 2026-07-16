/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"

static const ColonOptArg opt_args[] = {
    {'d', NULL},
    {'x', NULL},
    {'P', NULL},
    {'I', NULL},
    {'O', NULL},
    {'W', NULL},
    {0, NULL},
};

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-unzip",
        .colon = ":unzip",
        .real_cmd = "unzip",
        .style = COLON_STYLE_UNZIP,
        .rewrite = rewrite_unzip,
        .short_arg_opts = "dxPIOW",
        .opt_args = opt_args,
        .usage_extra =
            "  :unzip a.zip a/b:/c/d/e\n"
            "      extract archive members a/b/c/d/e/**, write to c/d/e/**",
    };
    return colon_cmd_main(&desc, argc, argv);
}
