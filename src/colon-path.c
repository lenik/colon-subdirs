/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-path.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

/*
 * Find colon-path separator. Returns pointer to ':', or NULL.
 * Leading ':' (empty BASE) counts — treated as BASE "/".
 * Skip host:path and host::module.
 */
static const char *find_colon_sep(const char *s) {
    if (!s || !*s || looks_like_uri(s)) {
        return NULL;
    }

    for (const char *p = s; *p; p++) {
        if (p[0] == ':' && p[1] == ':') {
            /* rsync daemon host::module — not a colon-path */
            return NULL;
        }
        if (*p != ':') {
            continue;
        }
        if (p == s) {
            /* :foo/bar → empty left, same as /:foo/bar */
            return p;
        }

        size_t prefix_len = (size_t)(p - s);
        int has_slash = 0;
        for (size_t i = 0; i < prefix_len; i++) {
            if (s[i] == '/') {
                has_slash = 1;
                break;
            }
        }

        int is_dot = (prefix_len == 1 && s[0] == '.') ||
                     (prefix_len == 2 && s[0] == '.' && s[1] == '.');

        if (has_slash || is_dot) {
            return p;
        }
        /* no slash in prefix → treat as host:path (scp/rsync) */
        return NULL;
    }
    return NULL;
}

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) {
        memcpy(d, s, n);
    }
    return d;
}

static void strip_leading_slashes(char *s) {
    char *p = s;
    while (*p == '/') {
        p++;
    }
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }
}

int colon_path_parse(ColonPath *out, const char *arg) {
    memset(out, 0, sizeof(*out));
    if (!arg) {
        errno = EINVAL;
        return -1;
    }

    out->input = xstrdup(arg);
    if (!out->input) {
        return -1;
    }

    const char *sep = find_colon_sep(arg);
    if (!sep) {
        out->is_colon = 0;
        out->base = NULL;
        out->leaf = xstrdup(arg);
        out->physical = xstrdup(arg);
        if (!out->leaf || !out->physical) {
            colon_path_free(out);
            return -1;
        }
        return 0;
    }

    out->is_colon = 1;
    size_t base_len = (size_t)(sep - arg);
    if (base_len == 0) {
        /* :foo/bar → BASE "/" */
        out->base = xstrdup("/");
    } else {
        out->base = malloc(base_len + 1);
        if (!out->base) {
            colon_path_free(out);
            return -1;
        }
        memcpy(out->base, arg, base_len);
        out->base[base_len] = '\0';
    }
    if (!out->base) {
        colon_path_free(out);
        return -1;
    }

    out->leaf = xstrdup(sep + 1);
    if (!out->leaf) {
        colon_path_free(out);
        return -1;
    }
    strip_leading_slashes(out->leaf);

    out->physical = colon_join(out->base, out->leaf);
    if (!out->physical) {
        colon_path_free(out);
        return -1;
    }
    return 0;
}

void colon_path_free(ColonPath *p) {
    if (!p) {
        return;
    }
    free(p->input);
    free(p->base);
    free(p->leaf);
    free(p->physical);
    memset(p, 0, sizeof(*p));
}

char *colon_subst_slash(const char *arg) {
    if (!arg) {
        errno = EINVAL;
        return NULL;
    }
    const char *sep = strchr(arg, ':');
    if (!sep) {
        return xstrdup(arg);
    }
    /* Do not touch URI schemes or host::module */
    if (looks_like_uri(arg) || strstr(arg, "::")) {
        return xstrdup(arg);
    }
    size_t n = strlen(arg);
    char *r = malloc(n + 1);
    if (!r) {
        return NULL;
    }
    memcpy(r, arg, n + 1);
    r[sep - arg] = '/';
    return r;
}

char *colon_join(const char *dir, const char *leaf) {
    if (!dir || !leaf) {
        errno = EINVAL;
        return NULL;
    }
    if (!*leaf) {
        return xstrdup(dir);
    }
    if (!*dir || strcmp(dir, ".") == 0) {
        return xstrdup(leaf);
    }

    size_t dl = strlen(dir);
    size_t ll = strlen(leaf);
    int need_slash = (dl > 0 && dir[dl - 1] != '/');
    char *r = malloc(dl + (need_slash ? 1 : 0) + ll + 1);
    if (!r) {
        return NULL;
    }
    memcpy(r, dir, dl);
    size_t o = dl;
    if (need_slash) {
        r[o++] = '/';
    }
    memcpy(r + o, leaf, ll + 1);
    return r;
}

char *colon_dirname(const char *path) {
    if (!path || !*path) {
        return xstrdup(".");
    }
    const char *slash = strrchr(path, '/');
    if (!slash) {
        return xstrdup(".");
    }
    if (slash == path) {
        return xstrdup("/");
    }
    size_t n = (size_t)(slash - path);
    char *d = malloc(n + 1);
    if (!d) {
        return NULL;
    }
    memcpy(d, path, n);
    d[n] = '\0';
    return d;
}

char *colon_basename(const char *path) {
    if (!path) {
        return NULL;
    }
    const char *slash = strrchr(path, '/');
    if (!slash) {
        return xstrdup(path);
    }
    return xstrdup(slash + 1);
}

int colon_mkdir_parents(const char *path) {
    char *dir = colon_dirname(path);
    if (!dir) {
        return -1;
    }
    if (strcmp(dir, ".") == 0 || strcmp(dir, "/") == 0) {
        free(dir);
        return 0;
    }

    /* Walk and mkdir each component. */
    char *tmp = xstrdup(dir);
    if (!tmp) {
        free(dir);
        return -1;
    }

    char *p = tmp;
    if (p[0] == '/') {
        p++;
    }
    for (; *p; p++) {
        if (*p != '/') {
            continue;
        }
        *p = '\0';
        if (tmp[0] != '\0' && mkdir(tmp, 0777) != 0 && errno != EEXIST) {
            free(tmp);
            free(dir);
            return -1;
        }
        *p = '/';
    }
    if (mkdir(tmp, 0777) != 0 && errno != EEXIST) {
        free(tmp);
        free(dir);
        return -1;
    }
    free(tmp);
    free(dir);
    return 0;
}

int colon_is_dir(const char *path) {
    if (!path || !*path) {
        return 0;
    }
    size_t n = strlen(path);
    if (path[n - 1] == '/') {
        return 1;
    }
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}
