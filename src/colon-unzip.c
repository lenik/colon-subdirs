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

static const char options_help[] =
    "Options:\n"
    "  -l, --list              list archive contents\n"
    "  -t, --test              test archive integrity\n"
    "  -v, --verbose           verbose list or test\n"
    "  -z, --archive-comment   show archive comment only\n"
    "  -p, --extract-to-stdout extract files to stdout\n"
    "  -c, --extract-to-stdout extract to stdout (alias)\n"
    "  -f, --force             freshen existing files only\n"
    "  -u, --update            update only if archive member is newer\n"
    "  -n, --never-overwrite   never overwrite existing files\n"
    "  -o, --overwrite         overwrite existing files without prompting\n"
    "  -j, --junk-paths        junk directory paths when extracting\n"
    "  -d, --directory=DIR     extract files into DIR\n"
    "  -x, --exclude=GLOB      exclude files matching GLOB\n"
    "  -q, --quiet             quiet mode\n"
    "  -qq                     very quiet mode\n"
    "  -P, --password=PASS     decrypt encrypted entries\n"
    "  -I, --charset=CHARSET   charset for zip entry names\n"
    "  -O, --output-dir=DIR    output directory for extracted files\n"
    "  -W, --workdir=DIR       use DIR as temporary work directory\n"
    "\n"
    "Colon-path members extract to LEAF paths. COLON_WRAP=1 passes all\n"
    "options to the PATH unzip.\n";

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
        .options_help = options_help,
    };
    return colon_cmd_main(&desc, argc, argv);
}
