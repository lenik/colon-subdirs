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

static const char options_help[] =
    "Options:\n"
    "  -a, --archive           archive mode (preserves most attributes)\n"
    "  -v, --verbose           increase verbosity\n"
    "  -z, --compress          compress file data during transfer\n"
    "  -r, --recursive         recurse into directories\n"
    "  -l, --links             copy symlinks as symlinks\n"
    "  -p, --perms             preserve permissions\n"
    "  -t, --times             preserve modification times\n"
    "  -g, --group             preserve group\n"
    "  -o, --owner             preserve owner (super-user only)\n"
    "  -D, --devices           preserve device files (super-user only)\n"
    "  -H, --hard-links        preserve hard links\n"
    "  -A, --acls              preserve ACLs\n"
    "  -X, --xattrs            preserve extended attributes\n"
    "  -e, --rsh=COMMAND       specify remote shell (or --rsh=COMMAND)\n"
    "      --delete            delete extraneous files from destination\n"
    "      --exclude=PATTERN   exclude files matching PATTERN\n"
    "      --include=PATTERN   include files matching PATTERN\n"
    "      --progress          show progress during transfer\n"
    "  -n, --dry-run           perform a trial run with no changes made\n"
    "      --dry-run           perform a trial run with no changes made\n"
    "  -h, --help              show rsync help (not wrapper help)\n"
    "\n"
    "Local colon-path operands use BASE:LEAF when BASE contains '/'.\n"
    "Remote host:path and host::module require COLON_WRAP=1 (or COLON_WRAP=rsync).\n";

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
        .options_help = options_help,
    };
    return colon_cmd_main(&desc, argc, argv);
}
