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

static const char options_help[] =
    "Options:\n"
    "  -f, --force             ignore nonexistent files, never prompt\n"
    "  -i                      prompt before every removal\n"
    "  -I                      prompt once before removing more than three\n"
    "                          files, or when removing recursively\n"
    "      --interactive[=WHEN]  prompt according to WHEN: never, once (-I),\n"
    "                          or always (-i); without WHEN, prompt always\n"
    "  -r, -R, --recursive     remove directories and their contents recursively\n"
    "  -d, --dir               remove empty directories (like rmdir)\n"
    "  -v, --verbose           explain what is being done\n"
    "      --one-file-system   when removing recursively, skip mount points\n"
    "      --no-preserve-root  do not treat '/' specially\n"
    "      --preserve-root[=all]  do not remove '/' (default)\n"
    "\n"
    "After removing a colon-path target, empty parent directories along LEAF\n"
    "are removed when possible (BASE is never removed).\n"
    "Built-in rewrite supports -f/-r/-R/-v. COLON_WRAP=1 passes all opts to rm.\n";

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
            "Examples:\n"
            "  :rm a/b:c/d       →  rm a/b/c/d\n"
            "  :rm .:d/e/f       →  rm d/e/f, then rmdir empty d/e and d",
        .options_help = options_help,
    };
    return colon_cmd_main(&desc, argc, argv);
}
