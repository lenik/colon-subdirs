/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Local-only scp: recursive copy like cp -r when both sides are local paths.
 */

#define _DEFAULT_SOURCE

#include "rewrite.h"
#include "rewrite/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int looks_like_uri(const char *s) {
    const char *p = strstr(s, "://");
    if (!p) {
        return 0;
    }
    for (const char *q = s; q < p; q++) {
        if (!((*q >= 'a' && *q <= 'z') || (*q >= 'A' && *q <= 'Z') ||
              (*q >= '0' && *q <= '9') || *q == '+' || *q == '-' || *q == '.')) {
            return 0;
        }
    }
    return p > s;
}

static int is_remote_scp_path(const char *s) {
    if (!s || looks_like_uri(s)) {
        return 0;
    }
    const char *colon = strchr(s, ':');
    if (!colon) {
        return 0;
    }
    if (colon[1] == ':') {
        return 0;
    }
    for (const char *p = s; p < colon; p++) {
        if (*p == '/') {
            return 0;
        }
    }
    return 1;
}

static int short_opt_takes_arg(const char *a) {
    if (a[0] != '-' || !a[1] || a[1] == '-') {
        return 0;
    }
    for (const char *p = a + 1; *p; p++) {
        if (strchr("cFiJloPS", *p)) {
            return 1;
        }
    }
    return 0;
}

static int scan_remote_operands(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "--") == 0) {
            for (i++; i < argc; i++) {
                if (is_remote_scp_path(argv[i])) {
                    return 1;
                }
            }
            break;
        }
        if (a[0] == '-' && a[1]) {
            if (short_opt_takes_arg(a) && i + 1 < argc) {
                i++;
            }
            continue;
        }
        if (is_remote_scp_path(a)) {
            return 1;
        }
    }
    return 0;
}

int rewrite_scp(int argc, char **argv) {
    const char *prog = argv[0] ? argv[0] : "scp";

    if (scan_remote_operands(argc, argv)) {
        fprintf(stderr,
                "%s: remote host:path is not supported; unset COLON_WRAP or set "
                "COLON_WRAP to use external scp\n",
                prog);
        return 1;
    }

    char **cpv = malloc((size_t)(argc + 2) * sizeof(char *));
    if (!cpv) {
        return 1;
    }
    int j = 0;
    cpv[j++] = argv[0];
    cpv[j++] = (char *)"-r";
    for (int i = 1; i < argc; i++) {
        cpv[j++] = argv[i];
    }
    int rc = rewrite_cp(j, cpv);
    free(cpv);
    return rc;
}
