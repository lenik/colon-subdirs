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

static const char options_help[] =
    "Options:\n"
    "  -b, --backup[=CONTROL]  make a backup of each existing destination file\n"
    "  -f, --force             do not prompt before overwriting\n"
    "  -i, --interactive       prompt before overwrite\n"
    "  -n, --no-clobber        do not overwrite an existing file\n"
    "  -t, --target-directory=DIR  move all sources into DIR\n"
    "  -T, --no-target-directory   treat DEST as a normal file\n"
    "  -u, --update[=WHEN]     move only when SOURCE is newer (or dest missing)\n"
    "  -v, --verbose           explain what is being done\n"
    "  -S, --suffix=SUFFIX     override the usual backup suffix\n"
    "      --strip-trailing-slashes  remove trailing slashes from each SOURCE\n"
    "  -Z, --context           set SELinux context of destination to default\n"
    "\n"
    "With multiple sources, or when DEST is a directory, each source is placed\n"
    "at DEST/LEAF (colon-path leaf, or basename). Parent dirs are created.\n"
    "Built-in rewrite supports a subset of options. COLON_WRAP=1 passes all\n"
    "options to the PATH mv.\n";

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-mv",
        .colon = ":mv",
        .real_cmd = "mv",
        .style = COLON_STYLE_SRCDEST,
        .rewrite = rewrite_mv,
        .short_arg_opts = "St",
        .opt_args = opt_args,
        .usage_extra =
            "Examples:\n"
            "  :mv a/b:c/d ../x     →  mv a/b/c/d ../x/c/d  (mkdir ../x/c)\n"
            "  :mv a/b:c/d e:f/g    →  mv a/b/c/d e/f/g\n"
            "  :mv a/b:c/d :tmp/out →  mv a/b/c/d /tmp/out",
        .options_help = options_help,
    };
    return colon_cmd_main(&desc, argc, argv);
}
