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

static const char options_help[] =
    "Options:\n"
    "  -r, --recurse-paths     recurse into directories\n"
    "  -q, --quiet             quiet mode\n"
    "  -u, --update            update only if archive member is newer\n"
    "  -d, --delete            delete entries in zipfile that are not on disk\n"
    "  -m, --move              move files into zipfile (delete originals)\n"
    "  -j, --junk-paths        store only the basename of each file\n"
    "  -y, --symlinks          store symlinks as the link instead of the file\n"
    "  -x, --exclude=GLOB      exclude files matching GLOB\n"
    "  -i, --include=GLOB      include only files matching GLOB\n"
    "  -0..-9                  compression level (0=store, 9=best)\n"
    "  -e, --encrypt           encrypt archive entries\n"
    "  -P, --password=PASS     set password for encryption\n"
    "  -F, --fix               try to repair a damaged archive\n"
    "  -FF                     try harder to repair a damaged archive\n"
    "  -A, --adjust-suffix     adjust self-extracting exe suffix\n"
    "  -FS, --filesync         sync changed files only\n"
    "  -X, --no-extra          do not save extra file attributes\n"
    "  -g, --grow              grow existing archive\n"
    "  -o, --latest-time       set zip entry time to newest file time\n"
    "  -b, --temp-path=DIR     use DIR for temporary files\n"
    "  -n, --suffix=SUFFIX     suffix for backup files\n"
    "  -t, --test              test archive integrity\n"
    "\n"
    "Colon-path sources are stored under LEAF names. COLON_WRAP=1 passes all\n"
    "options to the PATH zip.\n";

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-zip",
        .colon = ":zip",
        .real_cmd = "zip",
        .style = COLON_STYLE_ZIP,
        .rewrite = rewrite_zip,
        /* zip option letters that take a separate argument (common set) */
        .short_arg_opts = "bntdP",
        .opt_args = opt_args,
        .usage_extra =
            "  :zip a.zip a/b:c/d/e\n"
            "      archive members are stored as c/d/e/... (not a/b/c/d/e/...)",
        .options_help = options_help,
    };
    return colon_cmd_main(&desc, argc, argv);
}
