/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Local-only rsync: copy sources into destination like cp -a (recursive).
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

static int is_remote_rsync_path(const char *s) {
    if (!s || looks_like_uri(s)) {
        return 0;
    }
    if (strstr(s, "::") != NULL) {
        return 1;
    }
    const char *colon = strchr(s, ':');
    if (!colon) {
        return 0;
    }
    for (const char *p = s; p < colon; p++) {
        if (*p == '/') {
            return 0;
        }
    }
    return 1;
}

static int long_opt_takes_arg(const char *a) {
    static const char *opts[] = {
        "rsh", "e", "files-from", "exclude", "exclude-from", "include", "include-from",
        "filter", "partial-dir", "bwlimit", "timeout", "contimeout", "out-format", "log-file",
        "password-file", "protocol", "iconv", "checksum-seed", "max-size", "min-size",
        "max-delete", "block-size", "modify-window", "temp-dir", "compare-dest", "copy-dest",
        "link-dest", "chmod", "backup-dir", "suffix", NULL,
    };
    const char *name = a + 2;
    const char *eq = strchr(name, '=');
    size_t nlen = eq ? (size_t)(eq - name) : strlen(name);
    for (size_t i = 0; opts[i]; i++) {
        if (strlen(opts[i]) == nlen && strncmp(name, opts[i], nlen) == 0) {
            return eq == NULL;
        }
    }
    return 0;
}

static int short_cluster_takes_arg(const char *a) {
    for (const char *p = a + 1; *p; p++) {
        if (*p == 'e') {
            return 1;
        }
    }
    return 0;
}

static int collect_paths(int argc, char **argv, char ***out, int *nout) {
    char **v = NULL;
    int n = 0;
    int cap = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "--") == 0) {
            for (i++; i < argc; i++) {
                if (n + 1 >= cap) {
                    int nc = cap ? cap * 2 : 8;
                    char **nv = realloc(v, (size_t)nc * sizeof(char *));
                    if (!nv) {
                        free(v);
                        return -1;
                    }
                    v = nv;
                    cap = nc;
                }
                v[n++] = argv[i];
            }
            break;
        }
        if (a[0] == '-' && a[1]) {
            if (strncmp(a, "--", 2) == 0) {
                if (long_opt_takes_arg(a) && strchr(a, '=') == NULL && i + 1 < argc) {
                    i++;
                }
            } else if (short_cluster_takes_arg(a) && i + 1 < argc) {
                i++;
            }
            continue;
        }
        if (n + 1 >= cap) {
            int nc = cap ? cap * 2 : 8;
            char **nv = realloc(v, (size_t)nc * sizeof(char *));
            if (!nv) {
                free(v);
                return -1;
            }
            v = nv;
            cap = nc;
        }
        v[n++] = (char *)a;
    }

    *out = v;
    *nout = n;
    return 0;
}

int rewrite_rsync(int argc, char **argv) {
    const char *prog = argv[0] ? argv[0] : "rsync";
    char **paths = NULL;
    int npaths = 0;

    if (collect_paths(argc, argv, &paths, &npaths) != 0) {
        return 1;
    }

    for (int i = 0; i < npaths; i++) {
        if (is_remote_rsync_path(paths[i])) {
            fprintf(stderr,
                    "%s: remote rsync (host:path or host::module) requires external "
                    "rsync via COLON_WRAP\n",
                    prog);
            free(paths);
            return 1;
        }
    }

    if (npaths < 2) {
        fprintf(stderr, "%s: missing destination\n", prog);
        free(paths);
        return 1;
    }

    char **cpv = malloc((size_t)(npaths + 3) * sizeof(char *));
    if (!cpv) {
        free(paths);
        return 1;
    }
    int j = 0;
    cpv[j++] = argv[0];
    cpv[j++] = (char *)"-a";
    for (int i = 0; i < npaths; i++) {
        cpv[j++] = paths[i];
    }
    free(paths);

    int rc = rewrite_cp(j, cpv);
    free(cpv);
    return rc;
}
