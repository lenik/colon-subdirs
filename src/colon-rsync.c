/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"

/* Common rsync options that take an argument (subset; others still pass through
 * when attached as --opt=value). */
static const ColonOptArg opt_args[] = {
    {0, "rsh"},
    {0, "e"},
    {'e', NULL},
    {0, "files-from"},
    {0, "exclude"},
    {0, "exclude-from"},
    {0, "include"},
    {0, "include-from"},
    {0, "filter"},
    {0, "partial-dir"},
    {0, "bwlimit"},
    {0, "timeout"},
    {0, "contimeout"},
    {0, "out-format"},
    {0, "log-file"},
    {0, "password-file"},
    {0, "protocol"},
    {0, "iconv"},
    {0, "checksum-seed"},
    {0, "max-size"},
    {0, "min-size"},
    {0, "max-delete"},
    {0, "block-size"},
    {0, "modify-window"},
    {0, "temp-dir"},
    {0, "compare-dest"},
    {0, "copy-dest"},
    {0, "link-dest"},
    {0, "chmod"},
    {0, "backup-dir"},
    {0, "suffix"},
    {0, NULL},
};

int main(int argc, char **argv) {
    static const ColonCmdDesc desc = {
        .prog = "colon-rsync",
        .colon = ":rsync",
        .real_cmd = "rsync",
        .style = COLON_STYLE_SRCDEST,
        .rewrite = rewrite_rsync,
        .short_arg_opts = "e",
        .opt_args = opt_args,
        .usage_extra =
            "  Colon-path uses BASE:LEAF when BASE contains '/'.\n"
            "  host:path and host::module are left for rsync.",
    };
    return colon_cmd_main(&desc, argc, argv);
}
