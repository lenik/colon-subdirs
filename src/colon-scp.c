/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"

static const ColonOptArg opt_args[] = {
    {'c', NULL},
    {'D', NULL},
    {'F', NULL},
    {'i', NULL},
    {'J', NULL},
    {'l', NULL},
    {'o', NULL},
    {'P', NULL},
    {'S', NULL},
    {'X', NULL},
    {0, NULL},
};

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-scp",
        .colon = ":scp",
        .real_cmd = "scp",
        .style = COLON_STYLE_SRCDEST,
        .rewrite = rewrite_scp,
        .short_arg_opts = "cDFiJloPSX",
        .opt_args = opt_args,
        .usage_extra =
            "  Colon-path uses BASE:LEAF when BASE contains '/'.\n"
            "  host:path forms (no '/' before ':') are left for scp.",
    };
    return colon_cmd_main(&desc, argc, argv);
}
