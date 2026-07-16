/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"

static const ColonOptArg opt_args[] = {
    {'S', "suffix"},
    {'t', "target-directory"},
    {0, "backup"},
    {0, "suffix"},
    {0, "target-directory"},
    {0, "update"},
    {0, NULL},
};

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-mv",
        .colon = ":mv",
        .real_cmd = "mv",
        .style = COLON_STYLE_SRCDEST,
        .short_arg_opts = "St",
        .opt_args = opt_args,
        .usage_extra =
            "  :mv a/b:c/d ../x    →  mv a/b/c/d ../x/c/d  (mkdir ../x/c)\n"
            "  :mv a/b:c/d e:f/g   →  mv a/b/c/d e/f/g",
    };
    return colon_cmd_main(&desc, argc, argv);
}
