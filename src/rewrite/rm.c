/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#define _DEFAULT_SOURCE

#include "rewrite.h"
#include "rewrite/util.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int force, recursive, verbose;

static int rm_path(const char *prog, const char *path);

static int rm_dir(const char *prog, const char *path) {
    DIR *d = opendir(path);
    if (!d) {
        if (force && errno == ENOENT) {
            return 0;
        }
        rw_perror(prog, path);
        return -1;
    }
    struct dirent *de;
    int rc = 0;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        char child[4096];
        snprintf(child, sizeof child, "%s/%s", path, de->d_name);
        if (rm_path(prog, child) != 0) {
            rc = -1;
        }
    }
    closedir(d);
    if (rmdir(path) != 0) {
        if (!(force && errno == ENOENT)) {
            rw_perror(prog, path);
            rc = -1;
        }
    } else if (verbose) {
        printf("removed directory '%s'\n", path);
    }
    return rc;
}

static int rm_path(const char *prog, const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        if (force && errno == ENOENT) {
            return 0;
        }
        rw_perror(prog, path);
        return -1;
    }
    if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
        if (!recursive) {
            fprintf(stderr, "%s: cannot remove '%s': Is a directory\n", prog, path);
            return -1;
        }
        return rm_dir(prog, path);
    }
    if (unlink(path) != 0) {
        if (!(force && errno == ENOENT)) {
            rw_perror(prog, path);
            return -1;
        }
    } else if (verbose) {
        printf("removed '%s'\n", path);
    }
    return 0;
}

int rewrite_rm(int argc, char **argv) {
    const char *prog = argv[0] ? argv[0] : "rm";
    force = recursive = verbose = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][1]) {
        if (strcmp(argv[i], "--") == 0) {
            i++;
            break;
        }
        if (argv[i][1] == '-') {
            i++;
            continue;
        }
        for (char *p = argv[i] + 1; *p; p++) {
            if (*p == 'f') {
                force = 1;
            } else if (*p == 'r' || *p == 'R') {
                recursive = 1;
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
        if (rm_path(prog, argv[i]) != 0) {
            rc = 1;
        }
    }
    return rc;
}
