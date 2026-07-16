/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Built-in mv: rename(2), cross-device copy+unlink. Options: -f -i -n -T -v -t DIR
 */

#define _DEFAULT_SOURCE

#include "rewrite.h"
#include "rewrite/util.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int force, interactive, no_clobber, no_target, verbose;
static const char *target_dir;

static int parse_opts(int *argc, char ***argv) {
    char **av = *argv;
    int ac = *argc;
    int i = 1;
    while (i < ac) {
        char *a = av[i];
        if (strcmp(a, "--") == 0) {
            i++;
            break;
        }
        if (a[0] != '-' || a[1] == '\0') {
            break;
        }
        if (strcmp(a, "-t") == 0 || strncmp(a, "--target-directory", 18) == 0) {
            if (strcmp(a, "-t") == 0 || strcmp(a, "--target-directory") == 0) {
                if (i + 1 >= ac) {
                    return -1;
                }
                target_dir = av[++i];
            } else if (strncmp(a, "--target-directory=", 19) == 0) {
                target_dir = a + 19;
            }
            i++;
            continue;
        }
        if (strcmp(a, "-T") == 0 || strcmp(a, "--no-target-directory") == 0) {
            no_target = 1;
            i++;
            continue;
        }
        if (a[1] == '-') {
            i++;
            continue; /* ignore unknown long opts */
        }
        for (char *p = a + 1; *p; p++) {
            switch (*p) {
            case 'f':
                force = 1;
                interactive = 0;
                no_clobber = 0;
                break;
            case 'i':
                interactive = 1;
                force = 0;
                break;
            case 'n':
                no_clobber = 1;
                force = 0;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'T':
                no_target = 1;
                break;
            default:
                break;
            }
        }
        i++;
    }
    *argv = av + i;
    *argc = ac - i;
    return 0;
}

static int confirm(const char *dst) {
    if (!interactive) {
        return 1;
    }
    fprintf(stderr, "overwrite '%s'? ", dst);
    int c = getchar();
    int yes = (c == 'y' || c == 'Y');
    while (c != '\n' && c != EOF) {
        c = getchar();
    }
    return yes;
}

static int copy_file(const char *src, const char *dst) {
    int in = open(src, O_RDONLY);
    if (in < 0) {
        return -1;
    }
    struct stat st;
    if (fstat(in, &st) != 0) {
        close(in);
        return -1;
    }
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (out < 0) {
        close(in);
        return -1;
    }
    int r = rw_copy_fd(in, out);
    close(in);
    if (close(out) != 0) {
        r = -1;
    }
    return r;
}

static int copy_tree(const char *src, const char *dst);

static int rm_tree(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        return -1;
    }
    if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) {
            return -1;
        }
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
                continue;
            }
            char child[4096];
            snprintf(child, sizeof child, "%s/%s", path, de->d_name);
            if (rm_tree(child) != 0) {
                closedir(d);
                return -1;
            }
        }
        closedir(d);
        return rmdir(path);
    }
    return unlink(path);
}

static int move_one(const char *prog, const char *src, const char *dst) {
    if (no_clobber && access(dst, F_OK) == 0) {
        return 0;
    }
    if (access(dst, F_OK) == 0) {
        if (!force && !confirm(dst)) {
            return 0;
        }
        if (rw_is_dir(dst)) {
            /* leave to rename failure / replace file only */
        } else if (unlink(dst) != 0 && !force) {
            rw_perror(prog, dst);
            return -1;
        }
    }

    char *dparent = strdup(dst);
    if (!dparent) {
        return -1;
    }
    char *slash = strrchr(dparent, '/');
    if (slash && slash != dparent) {
        *slash = '\0';
        if (rw_mkdir_p(dparent) != 0) {
            free(dparent);
            rw_perror(prog, dst);
            return -1;
        }
    }
    free(dparent);

    if (rename(src, dst) == 0) {
        if (verbose) {
            printf("%s -> %s\n", src, dst);
        }
        return 0;
    }
    if (errno != EXDEV) {
        rw_perror(prog, src);
        return -1;
    }
    /* cross-device */
    if (rw_is_dir(src)) {
        if (copy_tree(src, dst) != 0) {
            return -1;
        }
    } else if (copy_file(src, dst) != 0) {
        rw_perror(prog, src);
        return -1;
    }
    /* remove source tree */
    if (rw_is_dir(src)) {
        if (rm_tree(src) != 0) {
            return -1;
        }
    } else if (unlink(src) != 0) {
        rw_perror(prog, src);
        return -1;
    }
    if (verbose) {
        printf("%s -> %s\n", src, dst);
    }
    return 0;
}

static int copy_tree(const char *src, const char *dst) {
    if (mkdir(dst, 0777) != 0 && errno != EEXIST) {
        return -1;
    }
    DIR *d = opendir(src);
    if (!d) {
        return -1;
    }
    struct dirent *de;
    int rc = 0;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        char sp[4096], dp[4096];
        snprintf(sp, sizeof sp, "%s/%s", src, de->d_name);
        snprintf(dp, sizeof dp, "%s/%s", dst, de->d_name);
        if (rw_is_dir(sp)) {
            if (copy_tree(sp, dp) != 0) {
                rc = -1;
                break;
            }
        } else if (copy_file(sp, dp) != 0) {
            rc = -1;
            break;
        }
    }
    closedir(d);
    return rc;
}

int rewrite_mv(int argc, char **argv) {
    const char *prog = argv[0] ? argv[0] : "mv";
    force = interactive = no_clobber = no_target = verbose = 0;
    target_dir = NULL;

    char **av = argv;
    int ac = argc;
    if (parse_opts(&ac, &av) != 0 || ac < 1) {
        fprintf(stderr, "%s: missing file operand\n", prog);
        return 1;
    }

    if (target_dir) {
        int rc = 0;
        for (int i = 0; i < ac; i++) {
            const char *base = strrchr(av[i], '/');
            base = base ? base + 1 : av[i];
            char dst[4096];
            snprintf(dst, sizeof dst, "%s/%s", target_dir, base);
            if (move_one(prog, av[i], dst) != 0) {
                rc = 1;
            }
        }
        return rc;
    }

    if (ac == 1) {
        fprintf(stderr, "%s: missing destination\n", prog);
        return 1;
    }

    const char *dest = av[ac - 1];
    int nsrc = ac - 1;
    int into = !no_target && (nsrc > 1 || rw_is_dir(dest) ||
                              (dest[0] && dest[strlen(dest) - 1] == '/'));

    if (!into && nsrc == 1) {
        return move_one(prog, av[0], dest) == 0 ? 0 : 1;
    }

    int rc = 0;
    for (int i = 0; i < nsrc; i++) {
        const char *base = strrchr(av[i], '/');
        base = base ? base + 1 : av[i];
        char dst[4096];
        snprintf(dst, sizeof dst, "%s/%s", dest, base);
        if (move_one(prog, av[i], dst) != 0) {
            rc = 1;
        }
    }
    return rc;
}
