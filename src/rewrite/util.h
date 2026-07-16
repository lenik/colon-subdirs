/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Shared helpers for src/rewrite/*.c
 */

#ifndef COLON_REWRITE_UTIL_H
#define COLON_REWRITE_UTIL_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static inline void rw_perror(const char *prog, const char *path) {
    fprintf(stderr, "%s: ", prog);
    perror(path);
}

static inline int rw_mkdir_p(const char *path) {
    char *tmp = strdup(path);
    if (!tmp) {
        return -1;
    }
    size_t len = strlen(tmp);
    if (len == 0) {
        free(tmp);
        return 0;
    }
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0777) != 0 && errno != EEXIST) {
                free(tmp);
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST) {
        free(tmp);
        return -1;
    }
    free(tmp);
    return 0;
}

static inline int rw_copy_fd(int in, int out) {
    char buf[8192];
    for (;;) {
        ssize_t n = read(in, buf, sizeof buf);
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            return 0;
        }
        char *p = buf;
        while (n > 0) {
            ssize_t w = write(out, p, (size_t)n);
            if (w < 0) {
                return -1;
            }
            p += w;
            n -= w;
        }
    }
}

static inline int rw_is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

#endif /* COLON_REWRITE_UTIL_H */
