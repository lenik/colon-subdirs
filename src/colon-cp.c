/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"

static const ColonOptArg opt_args[] = {
    {'S', "suffix"},
    {'t', "target-directory"},
    {'R', NULL},
    {0, "backup"},
    {0, "suffix"},
    {0, "target-directory"},
    {0, "update"},
    {0, "preserve"},
    {0, "no-preserve"},
    {0, "context"},
    {0, "parents"},
    {0, "keep-directory-symlink"},
    {0, "reflink"},
    {0, "sparse"},
    {0, "attributes-only"},
    {0, NULL},
};

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-cp",
        .colon = ":cp",
        .real_cmd = "cp",
        .style = COLON_STYLE_SRCDEST,
        .rewrite = rewrite_cp,
        .short_arg_opts = "StZ",
        .opt_args = opt_args,
        .usage_extra =
            "  :cp a/b:c/d ../x    →  cp a/b/c/d ../x/c/d  (mkdir ../x/c)",
    };
    return colon_cmd_main(&desc, argc, argv);
}
