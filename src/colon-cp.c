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

static const char options_help[] =
    "Options:\n"
    "  -a, --archive           same as -dR --preserve=all\n"
    "  -b, --backup[=CONTROL]  make a backup of each existing destination file\n"
    "  -d                      same as --no-dereference --preserve=links\n"
    "  -f, --force             if an existing destination cannot be opened,\n"
    "                          remove it and try again\n"
    "  -i, --interactive       prompt before overwrite\n"
    "  -l, --link              hard-link files instead of copying\n"
    "  -H, --dereference       follow command-line symbolic links\n"
    "  -L, --dereference       always follow symbolic links in SOURCE\n"
    "  -n, --no-clobber        do not overwrite an existing file\n"
    "  -P, --no-dereference    never follow symbolic links in SOURCE\n"
    "  -p                      same as --preserve=mode,ownership,timestamps\n"
    "      --preserve[=ATTR]   preserve attributes (default: mode,ownership,\n"
    "                          timestamps); ATTR may also include context,\n"
    "                          links, xattr, all\n"
    "  -r, -R, --recursive     copy directories recursively\n"
    "  -s, --symbolic-link     make symbolic links instead of copying\n"
    "  -S, --suffix=SUFFIX     override the usual backup suffix\n"
    "  -t, --target-directory=DIR  copy all sources into DIR\n"
    "  -T, --no-target-directory   treat DEST as a normal file\n"
    "  -u, --update[=WHEN]     copy only when SOURCE is newer (or dest missing)\n"
    "  -v, --verbose           explain what is being done\n"
    "  -x, --one-file-system   stay on this file system\n"
    "  -Z, --context           set SELinux security context of destination\n"
    "\n"
    "Built-in rewrite supports a subset (-f/-i/-n/-r/-R/-a/-v/-t/-T). When\n"
    "COLON_WRAP=1, all options are passed to the PATH cp.\n";

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
            "Examples:\n"
            "  :cp -a a/b:c/d ../x  →  cp -a a/b/c/d ../x/c/d",
        .options_help = options_help,
    };
    return colon_cmd_main(&desc, argc, argv);
}
