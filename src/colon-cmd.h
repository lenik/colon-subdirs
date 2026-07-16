/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef COLON_CMD_H
#define COLON_CMD_H

/*
 * How file arguments are rewritten before exec'ing the real tool.
 */
typedef enum ColonStyle {
    /* Sources..., DEST — append leaf under directory DEST (mv, cp, scp, rsync) */
    COLON_STYLE_SRCDEST = 0,
    /* All file args → physical paths only */
    COLON_STYLE_SOURCES,
    /* rm: remove targets, then rmdir empty leaf parents */
    COLON_STYLE_RM,
    /* rmdir: remove empty leaf dirs deepest-first (tip + empty parents) */
    COLON_STYLE_RMDIR,
    /* ZIPFILE SOURCES... — store under leaf names via staging (zip) */
    COLON_STYLE_ZIP,
    /* ZIPFILE [MEMBERS...] — extract physical members to leaf paths (unzip) */
    COLON_STYLE_UNZIP,
} ColonStyle;

/*
 * Option that takes a separate argument (short char, or 0 for long-only).
 * long_name without leading dashes; NULL-terminated list in desc.
 */
typedef struct ColonOptArg {
    int short_opt;         /* e.g. 't', or 0 */
    const char *long_name; /* e.g. "target-directory", or NULL */
} ColonOptArg;

typedef struct ColonCmdDesc {
    const char *prog;      /* "colon-mv" */
    const char *colon;     /* ":mv" */
    const char *real_cmd;  /* "mv" */
    ColonStyle style;
    /* Short options that take an argument (getopt-style string without ':') —
     * characters listed here consume the next argv element when seen as -X.
     * Long options with =arg or following arg listed in opt_args. */
    const char *short_arg_opts; /* e.g. "Stt" for mv — chars that take args */
    const ColonOptArg *opt_args; /* extra long opts that take args; may be NULL */
    const char *usage_extra;     /* one-line colon-path hint for --help */
} ColonCmdDesc;

/* Entry point used by each colon-* binary. Does not return on successful exec. */
int colon_cmd_main(const ColonCmdDesc *desc, int argc, char **argv);

#endif /* COLON_CMD_H */
