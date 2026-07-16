/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"

static const ColonOptArg opt_args[] = {
    {0, "interactive"},
    {0, "preserve-root"},
    {0, NULL},
};

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-rm",
        .colon = ":rm",
        .real_cmd = "rm",
        .style = COLON_STYLE_RM,
        .rewrite = rewrite_rm,
        .short_arg_opts = "",
        .opt_args = opt_args,
        .usage_extra =
            "  :rm a/b:c/d      →  rm a/b/c/d\n"
            "  :rm .:d/e/f      →  rm ./d/e/f, then rmdir empty d/e and d",
    };
    return colon_cmd_main(&desc, argc, argv);
}
