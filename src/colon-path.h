/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef COLON_PATH_H
#define COLON_PATH_H

#include <stddef.h>

/*
 * Colon-path: BASE:LEAF
 *
 *   physical  = BASE/LEAF   (actual filesystem / archive member path)
 *   leaf      = LEAF        (atomic name used for destinations / archive layout)
 *
 * Without a colon-path separator, physical and leaf both equal the input
 * (leaf for directory targets falls back to the final path component).
 *
 * A leading ':' (empty BASE) is treated as BASE "/":
 *   :foo/bar  ≡  /:foo/bar  →  physical /foo/bar
 *
 * A ':' is otherwise treated as a colon-path separator when the prefix contains
 * '/' or is "." / "..".  That avoids clashing with scp/rsync host:path.
 * rsync daemon "host::module" and URI schemes are never colon-paths.
 */

typedef struct ColonPath {
    char *input;     /* original argument (owned) */
    char *base;      /* before ':', or NULL; "/" when input was :LEAF */
    char *leaf;      /* atomic subpath (owned); no leading '/' */
    char *physical;  /* base/leaf joined, or input (owned) */
    int is_colon;    /* 1 if BASE:LEAF form was used */
} ColonPath;

/* Parse one argument. Returns 0 on success, -1 on OOM. */
int colon_path_parse(ColonPath *out, const char *arg);

void colon_path_free(ColonPath *p);

/*
 * For mv/cp destinations: replace the first ':' with '/'.
 *   :foo/bar → /foo/bar
 *   a/b:c/d  → a/b/c/d
 * No colon → strdup(arg). Caller frees. NULL on OOM.
 */
char *colon_subst_slash(const char *arg);

/* Join directory and leaf with '/'. Caller frees. NULL on OOM. */
char *colon_join(const char *dir, const char *leaf);

/* dirname of path (owned). "." if no slash. NULL on OOM. */
char *colon_dirname(const char *path);

/* basename of path (owned). NULL on OOM. */
char *colon_basename(const char *path);

/* mkdir -p for the parent directory of path. Returns 0 or -1. */
int colon_mkdir_parents(const char *path);

/* True if path exists and is a directory (trailing '/' also counts as intent). */
int colon_is_dir(const char *path);

#endif /* COLON_PATH_H */
