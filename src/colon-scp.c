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

static const char options_help[] =
    "Options:\n"
    "  -3                      route through the local host on both ends\n"
    "  -4                      force IPv4 addresses only\n"
    "  -6                      force IPv6 addresses only\n"
    "  -A                      forward agent authentication\n"
    "  -C                      enable compression\n"
    "  -O                      use legacy scp protocol\n"
    "  -p                      preserve modification times, modes, and xattrs\n"
    "  -q                      quiet mode\n"
    "  -r                      recursively copy directories\n"
    "  -T                      disable strict filename checking\n"
    "  -v                      verbose mode\n"
    "  -c CIPHER               select cipher\n"
    "  -F ssh_config           specify alternate ssh_config file\n"
    "  -i identity_file        select identity (private key) file\n"
    "  -J destination          jump host for ProxyJump\n"
    "  -l limit                limit bandwidth (Kbit/s)\n"
    "  -o ssh_option           pass ssh_option to ssh\n"
    "  -P port                 specify port (note uppercase P)\n"
    "  -S program              specify remote shell program\n"
    "\n"
    "Local colon-path operands use BASE:LEAF when BASE contains '/'.\n"
    "Remote host:path forms require COLON_WRAP=1 (or COLON_WRAP=scp).\n";

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
        .options_help = options_help,
    };
    return colon_cmd_main(&desc, argc, argv);
}
