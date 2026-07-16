/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _DEFAULT_SOURCE

#include "rewrite.h"
#include "rewrite/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int rewrite_rmdir(int argc, char **argv) {
    const char *prog = argv[0] ? argv[0] : "rmdir";
    int parents = 0;
    int verbose = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][1]) {
        if (strcmp(argv[i], "--") == 0) {
            i++;
            break;
        }
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--parents") == 0) {
            parents = 1;
            i++;
            continue;
        }
        if (argv[i][1] == '-') {
            i++;
            continue;
        }
        for (char *p = argv[i] + 1; *p; p++) {
            if (*p == 'p') {
                parents = 1;
            } else if (*p == 'v') {
                verbose = 1;
            }
        }
        i++;
    }
    if (i >= argc) {
        fprintf(stderr, "%s: missing operand\n", prog);
        return 1;
    }
    int rc = 0;
    for (; i < argc; i++) {
        char *path = strdup(argv[i]);
        if (!path) {
            return 1;
        }
        for (;;) {
            if (rmdir(path) != 0) {
                rw_perror(prog, path);
                rc = 1;
                break;
            }
            if (verbose) {
                printf("rmdir: removing directory, '%s'\n", path);
            }
            if (!parents) {
                break;
            }
            char *slash = strrchr(path, '/');
            if (!slash || slash == path) {
                break;
            }
            *slash = '\0';
            if (path[0] == '\0') {
                break;
            }
        }
        free(path);
    }
    return rc;
}
