/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "colon-cmd.h"
#include "colon-path.h"

#include "config.h"

#include <bas/locale/i18n.h>
#include <bas/log/deflog.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

define_logger();

static const char *g_argv0;

/* COLON_WRAP: unset → rewrite; "1" → PATH stem; else → exec that name */
static int wrap_is_rewrite(void) {
    const char *env = getenv("COLON_WRAP");
    return !env || env[0] == '\0';
}

static const char *wrap_exec_name(const ColonCmdDesc *desc) {
    const char *env = getenv("COLON_WRAP");
    if (env && env[0] && strcmp(env, "1") != 0) {
        return env;
    }
    /* COLON_WRAP=1 (or treated as stem): colon-mv / :mv → mv */
    const char *base = g_argv0 ? g_argv0 : desc->real_cmd;
    const char *slash = strrchr(base, '/');
    if (slash) {
        base = slash + 1;
    }
    if (strncmp(base, "colon-", 6) == 0) {
        return base + 6;
    }
    if (base[0] == ':' && base[1] != '\0') {
        return base + 1;
    }
    return desc->real_cmd;
}

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    if (d) {
        memcpy(d, s, n);
    }
    return d;
}

static int short_takes_arg(const ColonCmdDesc *desc, int c) {
    return desc->short_arg_opts && strchr(desc->short_arg_opts, c) != NULL;
}

static int long_takes_arg(const ColonCmdDesc *desc, const char *name, size_t nlen) {
    if (!desc->opt_args) {
        return 0;
    }
    for (const ColonOptArg *o = desc->opt_args; o->long_name || o->short_opt; o++) {
        if (o->long_name && strlen(o->long_name) == nlen &&
            strncmp(o->long_name, name, nlen) == 0) {
            return 1;
        }
    }
    return 0;
}

typedef struct ArgList {
    char **v;
    size_t n;
    size_t cap;
} ArgList;

static int arglist_push(ArgList *a, char *s) {
    if (a->n + 1 >= a->cap) {
        size_t nc = a->cap ? a->cap * 2 : 16;
        char **nv = realloc(a->v, nc * sizeof(char *));
        if (!nv) {
            return -1;
        }
        a->v = nv;
        a->cap = nc;
    }
    a->v[a->n++] = s;
    a->v[a->n] = NULL;
    return 0;
}

static void arglist_free(ArgList *a, int free_strings) {
    if (free_strings && a->v) {
        for (size_t i = 0; i < a->n; i++) {
            free(a->v[i]);
        }
    }
    free(a->v);
    memset(a, 0, sizeof(*a));
}

static int split_argv(const ColonCmdDesc *desc, int argc, char **argv, ArgList *opts,
                      ArgList *files, int *saw_help, int *saw_version, int *dry_run) {
    *saw_help = 0;
    *saw_version = 0;
    *dry_run = 0;
    int end_opts = 0;

    for (int i = 1; i < argc; i++) {
        char *a = argv[i];
        if (!end_opts && strcmp(a, "--") == 0) {
            end_opts = 1;
            if (arglist_push(opts, xstrdup("--")) != 0) {
                return -1;
            }
            continue;
        }
        if (!end_opts && a[0] == '-' && a[1] != '\0') {
            if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
                *saw_help = 1;
                continue;
            }
            if (strcmp(a, "--version") == 0) {
                *saw_version = 1;
                continue;
            }
            if (strcmp(a, "--colon-dry-run") == 0) {
                *dry_run = 1;
                continue;
            }

            if (a[1] == '-') {
                char *eq = strchr(a + 2, '=');
                size_t nlen = eq ? (size_t)(eq - (a + 2)) : strlen(a + 2);
                if (arglist_push(opts, xstrdup(a)) != 0) {
                    return -1;
                }
                if (!eq && long_takes_arg(desc, a + 2, nlen) && i + 1 < argc) {
                    i++;
                    if (arglist_push(opts, xstrdup(argv[i])) != 0) {
                        return -1;
                    }
                }
                continue;
            }

            if (arglist_push(opts, xstrdup(a)) != 0) {
                return -1;
            }
            const char *p = a + 1;
            while (*p) {
                int c = (unsigned char)*p++;
                if (short_takes_arg(desc, c)) {
                    if (*p != '\0') {
                        break;
                    }
                    if (i + 1 < argc) {
                        i++;
                        if (arglist_push(opts, xstrdup(argv[i])) != 0) {
                            return -1;
                        }
                    }
                    break;
                }
            }
            continue;
        }

        if (arglist_push(files, xstrdup(a)) != 0) {
            return -1;
        }
    }
    return 0;
}

static void print_help(const ColonCmdDesc *desc) {
    printf(_("Usage: %s [OPTION]... [FILE]...\n"), desc->colon);
    printf(_("       %s [OPTION]... [FILE]...\n"), desc->prog);
    printf(_("Colon-path aware %s.\n"), desc->real_cmd);
    fputc('\n', stdout);
    fputs(_("Colon-path syntax:\n"), stdout);
    fputs(_("  BASE:LEAF   physical path is BASE/LEAF; LEAF is the atomic name\n"), stdout);
    fputs(_("              used under a destination directory or inside archives.\n"), stdout);
    fputs(_("  :LEAF       empty BASE means \"/\" (same as /:LEAF).\n"), stdout);
    fputc('\n', stdout);
    if (desc->usage_extra) {
        fputs(desc->usage_extra, stdout);
        fputc('\n', stdout);
    }
    fputc('\n', stdout);
    fputs(_("Wrapper options:\n"), stdout);
    fputs(_("  -h, --help           show this help and exit\n"), stdout);
    fputs(_("      --version        show version and exit\n"), stdout);
    fputs(_("      --colon-dry-run  print planned command, do not execute\n"), stdout);
    fputc('\n', stdout);
    if (desc->options_help) {
        fputs(desc->options_help, stdout);
        fputc('\n', stdout);
    }
    fputs(_("Environment:\n"), stdout);
    fputs(_("  COLON_WRAP (unset)  use built-in rewrite implementation (default)\n"), stdout);
    fputs(_("  COLON_WRAP=1        exec stem of $0 from PATH (colon-mv → mv)\n"), stdout);
    fputs(_("  COLON_WRAP=<name>   exec <name> instead of the PATH tool\n"), stdout);
    fputc('\n', stdout);
    printf(_("Report bugs to: <%s>\n"), PROJECT_EMAIL);
}

static void print_version(const ColonCmdDesc *desc) {
    printf("%s %s\n", desc->prog, PROJECT_VERSION);
    printf(_("Copyright (C) %d %s\n"), PROJECT_YEAR, PROJECT_AUTHOR);
    fputs(_("License AGPL-3.0-or-later.\n"), stdout);
}

static int has_flag(ArgList *opts, char short_opt, const char *long_name) {
    for (size_t i = 0; i < opts->n; i++) {
        char *a = opts->v[i];
        if (long_name && strcmp(a, long_name) == 0) {
            return 1;
        }
        if (short_opt && a[0] == '-' && a[1] != '-') {
            for (char *p = a + 1; *p; p++) {
                if (*p == short_opt) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Remove -t/--target-directory and its value from opts (in place). */
static void strip_target_directory(ArgList *opts) {
    ArgList neu = {0};
    for (size_t i = 0; i < opts->n; i++) {
        char *a = opts->v[i];
        if (strcmp(a, "--target-directory") == 0) {
            free(a);
            if (i + 1 < opts->n) {
                free(opts->v[++i]);
            }
            continue;
        }
        if (strncmp(a, "--target-directory=", 19) == 0) {
            free(a);
            continue;
        }
        if (a[0] == '-' && a[1] != '-') {
            /* rewrite cluster removing 't'; drop following arg if t was last */
            int had_t = 0;
            int t_at_end = 0;
            for (char *p = a + 1; *p; p++) {
                if (*p == 't') {
                    had_t = 1;
                    t_at_end = (p[1] == '\0');
                    if (!t_at_end) {
                        /* -tVALUE → drop whole argv element */
                        had_t = 2;
                        break;
                    }
                }
            }
            if (had_t == 2) {
                free(a);
                continue;
            }
            if (had_t && t_at_end) {
                /* remove t from cluster */
                char *q = a + 1;
                for (char *p = a + 1; *p; p++) {
                    if (*p != 't') {
                        *q++ = *p;
                    }
                }
                *q = '\0';
                if (a[1] == '\0') {
                    free(a);
                    if (i + 1 < opts->n) {
                        free(opts->v[++i]);
                    }
                    continue;
                }
                if (i + 1 < opts->n) {
                    free(opts->v[++i]);
                }
                if (arglist_push(&neu, a) != 0) {
                    return;
                }
                continue;
            }
        }
        if (arglist_push(&neu, a) != 0) {
            return;
        }
    }
    free(opts->v);
    *opts = neu;
}


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

static void strip_leading_slashes(char *s) {
    char *p = s;
    while (*p == '/') {
        p++;
    }
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }
}

/*
 * Parse a path for SRC/DEST tools. scp/rsync keep host:path; local mv/cp treat
 * any single ':' (not URI, not host::module) as BASE:LEAF.
 */
static int parse_colon_arg(const ColonCmdDesc *desc, ColonPath *out, const char *arg) {
    int honor_host_colon =
        strcmp(desc->real_cmd, "scp") == 0 || strcmp(desc->real_cmd, "rsync") == 0;

    if (colon_path_parse(out, arg) != 0) {
        return -1;
    }
    if (out->is_colon || honor_host_colon) {
        return 0;
    }

    const char *sep = strchr(arg, ':');
    if (!sep || looks_like_uri(arg) || strstr(arg, "::")) {
        return 0;
    }

    colon_path_free(out);
    memset(out, 0, sizeof(*out));
    out->input = xstrdup(arg);
    if (!out->input) {
        return -1;
    }
    out->is_colon = 1;
    size_t base_len = (size_t)(sep - arg);
    if (base_len == 0) {
        out->base = xstrdup("/");
    } else {
        out->base = malloc(base_len + 1);
        if (out->base) {
            memcpy(out->base, arg, base_len);
            out->base[base_len] = '\0';
        }
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

static char *leaf_for_dest(const ColonPath *cp) {
    if (cp->is_colon) {
        return xstrdup(cp->leaf);
    }
    return colon_basename(cp->physical);
}

/*
 * Build SRC/DEST pairs in out_pairs (SRC DEST SRC DEST ...).
 * Returns number of pairs, or -1 on error.
 */
static int build_srcdest_pairs(const ColonCmdDesc *desc, ArgList *opts, ArgList *files,
                               ArgList *out_pairs) {
    char *target_dir = NULL;
    int has_t = 0;

    for (size_t i = 0; i < opts->n; i++) {
        char *a = opts->v[i];
        if (strcmp(a, "--target-directory") == 0 && i + 1 < opts->n) {
            target_dir = opts->v[i + 1];
            has_t = 1;
            break;
        }
        if (strncmp(a, "--target-directory=", 19) == 0) {
            target_dir = a + 19;
            has_t = 1;
            break;
        }
        if (a[0] == '-' && a[1] != '-') {
            for (char *p = a + 1; *p; p++) {
                if (*p == 't') {
                    if (p[1]) {
                        target_dir = p + 1;
                    } else if (i + 1 < opts->n) {
                        target_dir = opts->v[i + 1];
                    }
                    has_t = 1;
                    break;
                }
            }
            if (has_t) {
                break;
            }
        }
    }

    int no_target = has_flag(opts, 'T', "--no-target-directory");

    ColonPath dest_dir;
    memset(&dest_dir, 0, sizeof(dest_dir));
    size_t nsrc;
    const char *dest_base;

    int dest_use_subst =
        strcmp(desc->real_cmd, "mv") == 0 || strcmp(desc->real_cmd, "cp") == 0;

    if (has_t && target_dir) {
        if (parse_colon_arg(desc, &dest_dir, target_dir) != 0) {
            return -1;
        }
        if (dest_use_subst) {
            char *sub = colon_subst_slash(target_dir);
            if (!sub) {
                colon_path_free(&dest_dir);
                return -1;
            }
            free(dest_dir.physical);
            dest_dir.physical = sub;
        }
        dest_base = dest_dir.physical;
        nsrc = files->n;
        strip_target_directory(opts);
    } else {
        if (files->n < 1) {
            return 0;
        }
        if (parse_colon_arg(desc, &dest_dir, files->v[files->n - 1]) != 0) {
            return -1;
        }
        if (dest_use_subst) {
            char *sub = colon_subst_slash(files->v[files->n - 1]);
            if (!sub) {
                colon_path_free(&dest_dir);
                return -1;
            }
            free(dest_dir.physical);
            dest_dir.physical = sub;
        }
        dest_base = dest_dir.physical;
        nsrc = files->n - 1;
        if (nsrc == 0) {
            if (arglist_push(out_pairs, xstrdup(dest_dir.physical)) != 0) {
                colon_path_free(&dest_dir);
                return -1;
            }
            colon_path_free(&dest_dir);
            return 0;
        }
    }

    int into_dir = 0;
    if (has_t) {
        into_dir = 1;
    } else if (!no_target) {
        into_dir = (nsrc > 1) || colon_is_dir(dest_base) ||
                   (dest_dir.input && dest_dir.input[0] &&
                    dest_dir.input[strlen(dest_dir.input) - 1] == '/');
        /*
         * Colon-path source + plain DEST → treat DEST as a directory and
         * place at DEST/LEAF (even if DEST does not exist yet).
         * Both sides colon-path → exact rename to dest.physical (below).
         */
        if (!into_dir && !dest_dir.is_colon) {
            for (size_t i = 0; i < nsrc; i++) {
                ColonPath src;
                if (parse_colon_arg(desc, &src, files->v[i]) != 0) {
                    colon_path_free(&dest_dir);
                    return -1;
                }
                int is = src.is_colon;
                colon_path_free(&src);
                if (is) {
                    into_dir = 1;
                    break;
                }
            }
        }
    }

    if (!into_dir && nsrc == 1 && !has_t) {
        ColonPath src;
        if (parse_colon_arg(desc, &src, files->v[0]) != 0) {
            colon_path_free(&dest_dir);
            return -1;
        }
        char *ps = xstrdup(src.physical);
        char *pd = xstrdup(dest_dir.physical);
        colon_path_free(&src);
        colon_path_free(&dest_dir);
        if (!ps || !pd) {
            free(ps);
            free(pd);
            return -1;
        }
        if (colon_mkdir_parents(pd) != 0) {
            fprintf(stderr, "%s: %s: %s\n", desc->prog, pd, strerror(errno));
            free(ps);
            free(pd);
            return -1;
        }
        if (arglist_push(out_pairs, ps) != 0 || arglist_push(out_pairs, pd) != 0) {
            free(ps);
            free(pd);
            return -1;
        }
        return 1;
    }

    /* Directory destination: expand colon-path sources to DEST/LEAF.
     * Plain sources (including scp host:path) keep classic SRC... DEST form. */
    int any_colon = 0;
    for (size_t i = 0; i < nsrc; i++) {
        ColonPath src;
        if (parse_colon_arg(desc, &src, files->v[i]) != 0) {
            colon_path_free(&dest_dir);
            return -1;
        }
        if (src.is_colon) {
            any_colon = 1;
        }
        colon_path_free(&src);
        if (any_colon) {
            break;
        }
    }

    if (!any_colon) {
        for (size_t i = 0; i < nsrc; i++) {
            ColonPath src;
            if (parse_colon_arg(desc, &src, files->v[i]) != 0) {
                colon_path_free(&dest_dir);
                return -1;
            }
            char *ps = xstrdup(src.physical);
            colon_path_free(&src);
            if (!ps || arglist_push(out_pairs, ps) != 0) {
                free(ps);
                colon_path_free(&dest_dir);
                return -1;
            }
        }
        char *pd = xstrdup(dest_base);
        colon_path_free(&dest_dir);
        if (!pd || arglist_push(out_pairs, pd) != 0) {
            free(pd);
            return -1;
        }
        return 0; /* single exec: SRC... DEST */
    }

    for (size_t i = 0; i < nsrc; i++) {
        ColonPath src;
        if (parse_colon_arg(desc, &src, files->v[i]) != 0) {
            colon_path_free(&dest_dir);
            return -1;
        }
        char *leaf = leaf_for_dest(&src);
        char *dpath = leaf ? colon_join(dest_base, leaf) : NULL;
        free(leaf);
        if (!dpath) {
            colon_path_free(&src);
            colon_path_free(&dest_dir);
            return -1;
        }
        if (colon_mkdir_parents(dpath) != 0) {
            fprintf(stderr, "%s: %s: %s\n", desc->prog, dpath, strerror(errno));
            free(dpath);
            colon_path_free(&src);
            colon_path_free(&dest_dir);
            return -1;
        }
        char *ps = xstrdup(src.physical);
        colon_path_free(&src);
        if (!ps || arglist_push(out_pairs, ps) != 0 || arglist_push(out_pairs, dpath) != 0) {
            free(ps);
            free(dpath);
            colon_path_free(&dest_dir);
            return -1;
        }
    }
    colon_path_free(&dest_dir);
    return (int)nsrc;
}

static int rewrite_sources(ArgList *files, ArgList *out) {
    for (size_t i = 0; i < files->n; i++) {
        ColonPath p;
        if (colon_path_parse(&p, files->v[i]) != 0) {
            return -1;
        }
        char *s = xstrdup(p.physical);
        colon_path_free(&p);
        if (!s || arglist_push(out, s) != 0) {
            free(s);
            return -1;
        }
    }
    return 0;
}

static int run_exec(const ColonCmdDesc *desc, ArgList *opts, char **file_args, int dry_run);

static int path_is_base(const char *path, const char *base) {
    if (!path || !base) {
        return 0;
    }
    if (strcmp(path, base) == 0) {
        return 1;
    }
    /* base "." matches physical paths that walked up to "." */
    if (strcmp(base, ".") == 0 && (strcmp(path, ".") == 0 || strcmp(path, "./") == 0)) {
        return 1;
    }
    return 0;
}

/*
 * Best-effort rmdir of empty directories along LEAF under BASE.
 * If include_tip: also rmdir BASE/LEAF (deepest first).
 * Stops at BASE (never removes BASE). Non-empty dirs are skipped safely.
 * Returns non-zero only if include_tip and the tip rmdir failed.
 */
static int prune_leaf_dirs(const char *prog, const char *base, const char *leaf, int include_tip,
                           int dry_run) {
    if (!leaf || !*leaf) {
        return 0;
    }

    char *cur = colon_join(base, leaf);
    if (!cur) {
        return -1;
    }

    if (!include_tip) {
        /* Tip already removed (e.g. by rm); start at its parent. */
        char *parent = colon_dirname(cur);
        free(cur);
        cur = parent;
        if (!cur) {
            return -1;
        }
    }

    int is_tip = include_tip;
    int tip_failed = 0;

    while (cur && *cur && !path_is_base(cur, base)) {
        if (dry_run) {
            printf("rmdir -- %s\n", cur);
        } else if (rmdir(cur) != 0) {
            if (is_tip) {
                fprintf(stderr, "%s: failed to remove '%s': %s\n", prog, cur, strerror(errno));
                tip_failed = 1;
                free(cur);
                return tip_failed;
            }
            /* Best-effort empty-only for parents. */
            if (errno == ENOTEMPTY || errno == EEXIST || errno == EBUSY || errno == ENOENT ||
                errno == EPERM || errno == EACCES) {
                break;
            }
            break;
        }
        is_tip = 0;

        char *parent = colon_dirname(cur);
        free(cur);
        cur = parent;
        if (!cur) {
            return -1;
        }
        if (strcmp(cur, "/") == 0) {
            break;
        }
    }
    free(cur);
    return tip_failed;
}

/* Like run_exec but always fork/wait so the caller can continue (prune). */
static int run_exec_wait(const ColonCmdDesc *desc, ArgList *opts, char **file_args, int dry_run) {
    if (dry_run) {
        return run_exec(desc, opts, file_args, 1);
    }
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "%s: fork: %s\n", desc->prog, strerror(errno));
        return -1;
    }
    if (pid == 0) {
        _exit(run_exec(desc, opts, file_args, 0));
    }
    int st = 0;
    if (waitpid(pid, &st, 0) < 0) {
        fprintf(stderr, "%s: waitpid: %s\n", desc->prog, strerror(errno));
        return -1;
    }
    if (WIFEXITED(st)) {
        return WEXITSTATUS(st);
    }
    return 1;
}

typedef struct {
    char *base;
    char *leaf;
    int is_colon;
} LeafRef;

static void leafref_free(LeafRef *r) {
    free(r->base);
    free(r->leaf);
    memset(r, 0, sizeof(*r));
}

static int do_rm_or_rmdir(const ColonCmdDesc *desc, ArgList *opts, ArgList *files, int dry_run,
                          int is_rmdir) {
    if (files->n == 0) {
        return run_exec_wait(desc, opts, NULL, dry_run);
    }

    ArgList physicals = {0};
    LeafRef *refs = calloc(files->n, sizeof(LeafRef));
    if (!refs) {
        return -1;
    }

    int rc = 1;
    for (size_t i = 0; i < files->n; i++) {
        ColonPath p;
        /* Local rm/rmdir: allow BASE without '/' (.:d/e/f). */
        if (parse_colon_arg(desc, &p, files->v[i]) != 0) {
            goto out;
        }
        char *phys = xstrdup(p.physical);
        if (!phys || arglist_push(&physicals, phys) != 0) {
            free(phys);
            colon_path_free(&p);
            goto out;
        }
        if (p.is_colon) {
            refs[i].is_colon = 1;
            refs[i].base = p.base ? xstrdup(p.base) : xstrdup(".");
            refs[i].leaf = xstrdup(p.leaf);
            if (!refs[i].base || !refs[i].leaf) {
                colon_path_free(&p);
                goto out;
            }
        }
        colon_path_free(&p);
    }

    if (is_rmdir) {
        /*
         * Colon-path dirs: remove the whole empty LEAF chain ourselves
         * (deepest first). Plain args go through rmdir(1).
         */
        ArgList plain = {0};
        rc = 0;
        for (size_t i = 0; i < files->n; i++) {
            if (refs[i].is_colon) {
                int r = prune_leaf_dirs(desc->prog, refs[i].base, refs[i].leaf, 1, dry_run);
                if (r != 0) {
                    rc = r;
                }
            } else {
                if (arglist_push(&plain, xstrdup(physicals.v[i])) != 0) {
                    arglist_free(&plain, 1);
                    goto out;
                }
            }
        }
        if (plain.n > 0) {
            int r = run_exec_wait(desc, opts, plain.v, dry_run);
            if (r != 0) {
                rc = r;
            }
        } else if (dry_run && files->n == 0) {
            rc = run_exec_wait(desc, opts, NULL, dry_run);
        }
        arglist_free(&plain, 1);
    } else {
        /* rm tip(s) via rm(1), then prune empty leaf parents. */
        rc = run_exec_wait(desc, opts, physicals.v, dry_run);
        if (rc == 0 || rc == 1) {
            /* rm may return 1 for some failures on a subset; still try prune for
             * paths that are gone. Parents only (tip already handled by rm). */
            for (size_t i = 0; i < files->n; i++) {
                if (!refs[i].is_colon) {
                    continue;
                }
                if (dry_run || access(physicals.v[i], F_OK) != 0) {
                    prune_leaf_dirs(desc->prog, refs[i].base, refs[i].leaf, 0, dry_run);
                }
            }
        }
    }

out:
    for (size_t i = 0; i < files->n; i++) {
        leafref_free(&refs[i]);
    }
    free(refs);
    arglist_free(&physicals, 1);
    return rc;
}

static int run_exec(const ColonCmdDesc *desc, ArgList *opts, char **file_args, int dry_run) {
    ArgList argv = {0};
    const char *cmd_name = wrap_is_rewrite() ? desc->real_cmd : wrap_exec_name(desc);

    if (arglist_push(&argv, xstrdup(cmd_name)) != 0) {
        return -1;
    }
    for (size_t i = 0; i < opts->n; i++) {
        if (arglist_push(&argv, xstrdup(opts->v[i])) != 0) {
            arglist_free(&argv, 1);
            return -1;
        }
    }
    for (char **p = file_args; p && *p; p++) {
        if (arglist_push(&argv, xstrdup(*p)) != 0) {
            arglist_free(&argv, 1);
            return -1;
        }
    }

    if (dry_run) {
        if (wrap_is_rewrite()) {
            fputs("(rewrite) ", stdout);
        }
        for (size_t i = 0; i < argv.n; i++) {
            if (i) {
                fputc(' ', stdout);
            }
            fputs(argv.v[i], stdout);
        }
        fputc('\n', stdout);
        arglist_free(&argv, 1);
        return 0;
    }

    if (wrap_is_rewrite()) {
        if (!desc->rewrite) {
            fprintf(stderr, "%s: no rewrite implementation for %s\n", desc->prog, desc->real_cmd);
            arglist_free(&argv, 1);
            return 127;
        }
        int rc = desc->rewrite((int)argv.n, argv.v);
        arglist_free(&argv, 1);
        return rc;
    }

    execvp(cmd_name, argv.v);
    fprintf(stderr, "%s: %s: %s\n", desc->prog, cmd_name, strerror(errno));
    arglist_free(&argv, 1);
    return 127;
}

static int run_exec_pair(const ColonCmdDesc *desc, ArgList *opts, const char *src,
                         const char *dst, int dry_run) {
    char *files[3] = {(char *)src, (char *)dst, NULL};
    if (dry_run) {
        return run_exec(desc, opts, files, 1);
    }
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "%s: fork: %s\n", desc->prog, strerror(errno));
        return -1;
    }
    if (pid == 0) {
        _exit(run_exec(desc, opts, files, 0));
    }
    int st = 0;
    if (waitpid(pid, &st, 0) < 0) {
        fprintf(stderr, "%s: waitpid: %s\n", desc->prog, strerror(errno));
        return -1;
    }
    if (WIFEXITED(st)) {
        return WEXITSTATUS(st);
    }
    return 1;
}

/* ---- zip: stage leaf symlinks, zip from staging dir ---- */

static int rm_rf(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        return errno == ENOENT ? 0 : -1;
    }
    if (!S_ISDIR(st.st_mode) || S_ISLNK(st.st_mode)) {
        return unlink(path);
    }
    DIR *d = opendir(path);
    if (!d) {
        return -1;
    }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
            continue;
        }
        char *child = colon_join(path, de->d_name);
        if (!child) {
            closedir(d);
            return -1;
        }
        int r = rm_rf(child);
        free(child);
        if (r != 0) {
            closedir(d);
            return -1;
        }
    }
    closedir(d);
    return rmdir(path);
}

static int run_mv(const char *src, const char *dst) {
    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid == 0) {
        execlp("mv", "mv", "--", src, dst, (char *)NULL);
        _exit(127);
    }
    int st = 0;
    if (waitpid(pid, &st, 0) < 0) {
        return -1;
    }
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static int do_zip(const ColonCmdDesc *desc, ArgList *opts, ArgList *files, int dry_run) {
    if (files->n < 1) {
        fprintf(stderr, _("%s: missing zipfile operand\n"), desc->prog);
        return 1;
    }

    ColonPath zipfile;
    if (colon_path_parse(&zipfile, files->v[0]) != 0) {
        return -1;
    }

    /* Absolute zip path so chdir is safe */
    char zip_abs[4096];
    if (zipfile.physical[0] == '/') {
        snprintf(zip_abs, sizeof zip_abs, "%s", zipfile.physical);
    } else {
        char cwd[4096];
        if (!getcwd(cwd, sizeof cwd)) {
            colon_path_free(&zipfile);
            return -1;
        }
        snprintf(zip_abs, sizeof zip_abs, "%s/%s", cwd, zipfile.physical);
    }

    char template[] = "/tmp/colon-zip-XXXXXX";
    char *stagedir = mkdtemp(template);
    if (!stagedir) {
        fprintf(stderr, "%s: mkdtemp: %s\n", desc->prog, strerror(errno));
        colon_path_free(&zipfile);
        return -1;
    }
    stagedir = xstrdup(template);
    if (!stagedir) {
        colon_path_free(&zipfile);
        return -1;
    }

    ArgList leafs = {0};
    int rc = 1;

    for (size_t i = 1; i < files->n; i++) {
        ColonPath src;
        if (parse_colon_arg(desc, &src, files->v[i]) != 0) {
            goto out;
        }
        char *leaf = src.is_colon ? xstrdup(src.leaf) : xstrdup(src.physical);
        if (!leaf) {
            colon_path_free(&src);
            goto out;
        }

        char *linkpath = colon_join(stagedir, leaf);
        if (!linkpath) {
            free(leaf);
            colon_path_free(&src);
            goto out;
        }
        if (colon_mkdir_parents(linkpath) != 0) {
            fprintf(stderr, "%s: %s: %s\n", desc->prog, linkpath, strerror(errno));
            free(linkpath);
            free(leaf);
            colon_path_free(&src);
            goto out;
        }

        char phys_abs[4096];
        if (src.physical[0] == '/') {
            snprintf(phys_abs, sizeof phys_abs, "%s", src.physical);
        } else {
            char cwd[4096];
            if (!getcwd(cwd, sizeof cwd)) {
                free(linkpath);
                free(leaf);
                colon_path_free(&src);
                goto out;
            }
            snprintf(phys_abs, sizeof phys_abs, "%s/%s", cwd, src.physical);
        }

        if (symlink(phys_abs, linkpath) != 0) {
            fprintf(stderr, "%s: symlink %s: %s\n", desc->prog, linkpath, strerror(errno));
            free(linkpath);
            free(leaf);
            colon_path_free(&src);
            goto out;
        }
        free(linkpath);
        if (arglist_push(&leafs, leaf) != 0) {
            free(leaf);
            colon_path_free(&src);
            goto out;
        }
        colon_path_free(&src);
    }

    if (dry_run) {
        fputs(desc->real_cmd, stdout);
        for (size_t i = 0; i < opts->n; i++) {
            printf(" %s", opts->v[i]);
        }
        printf(" %s", zip_abs);
        for (size_t i = 0; i < leafs.n; i++) {
            printf(" %s", leafs.v[i]);
        }
        printf("  (from %s)\n", stagedir);
        rc = 0;
        goto out;
    }

    if (colon_mkdir_parents(zip_abs) != 0) {
        fprintf(stderr, "%s: %s: %s\n", desc->prog, zip_abs, strerror(errno));
        goto out;
    }

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "%s: fork: %s\n", desc->prog, strerror(errno));
        goto out;
    }
    if (pid == 0) {
        if (chdir(stagedir) != 0) {
            _exit(127);
        }
        ArgList argv = {0};
        const char *zcmd = wrap_is_rewrite() ? desc->real_cmd : wrap_exec_name(desc);
        arglist_push(&argv, xstrdup(zcmd));
        int have_r = 0;
        for (size_t i = 0; i < opts->n; i++) {
            if (strcmp(opts->v[i], "-r") == 0 || strcmp(opts->v[i], "-R") == 0 ||
                strncmp(opts->v[i], "-r", 2) == 0) {
                have_r = 1;
            }
            arglist_push(&argv, xstrdup(opts->v[i]));
        }
        if (!have_r) {
            /* Ensure directory trees are stored under leaf names. */
            arglist_push(&argv, xstrdup("-r"));
        }
        arglist_push(&argv, xstrdup(zip_abs));
        for (size_t i = 0; i < leafs.n; i++) {
            arglist_push(&argv, xstrdup(leafs.v[i]));
        }
        if (wrap_is_rewrite()) {
            if (!desc->rewrite) {
                _exit(127);
            }
            _exit(desc->rewrite((int)argv.n, argv.v));
        }
        execvp(zcmd, argv.v);
        _exit(127);
    }
    int st = 0;
    waitpid(pid, &st, 0);
    rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;

out:
    arglist_free(&leafs, 1);
    rm_rf(stagedir);
    free(stagedir);
    colon_path_free(&zipfile);
    return rc;
}

/* ---- unzip: extract members, relocate physical → leaf ---- */

static int do_unzip(const ColonCmdDesc *desc, ArgList *opts, ArgList *files, int dry_run) {
    if (files->n < 1) {
        fprintf(stderr, _("%s: missing zipfile operand\n"), desc->prog);
        return 1;
    }

    ColonPath zipfile;
    if (colon_path_parse(&zipfile, files->v[0]) != 0) {
        return -1;
    }

    /* No member args: pass through with physical zip path */
    if (files->n == 1) {
        char *fa[2] = {zipfile.physical, NULL};
        int r = run_exec(desc, opts, fa, dry_run);
        colon_path_free(&zipfile);
        return r;
    }

    char template[] = "/tmp/colon-unzip-XXXXXX";
    if (!mkdtemp(template)) {
        fprintf(stderr, "%s: mkdtemp: %s\n", desc->prog, strerror(errno));
        colon_path_free(&zipfile);
        return -1;
    }
    char *tmpdir = xstrdup(template);
    if (!tmpdir) {
        colon_path_free(&zipfile);
        return -1;
    }

    ArgList members = {0};
    typedef struct {
        char *physical;
        char *leaf;
    } Map;
    Map *maps = calloc(files->n - 1, sizeof(Map));
    size_t nmaps = 0;
    int rc = 1;

    if (!maps) {
        goto out;
    }

    for (size_t i = 1; i < files->n; i++) {
        ColonPath m;
        if (colon_path_parse(&m, files->v[i]) != 0) {
            goto out;
        }
        maps[nmaps].physical = xstrdup(m.physical);
        maps[nmaps].leaf = m.is_colon ? xstrdup(m.leaf) : xstrdup(m.physical);
        if (!maps[nmaps].physical || !maps[nmaps].leaf) {
            colon_path_free(&m);
            goto out;
        }
        /* Match the directory tree under the physical member prefix. */
        char *pat = NULL;
        size_t plen = strlen(m.physical) + 3;
        pat = malloc(plen);
        if (!pat) {
            colon_path_free(&m);
            goto out;
        }
        snprintf(pat, plen, "%s/*", m.physical);
        if (arglist_push(&members, pat) != 0) {
            free(pat);
            colon_path_free(&m);
            goto out;
        }
        nmaps++;
        colon_path_free(&m);
    }

    if (dry_run) {
        printf("unzip %s → tmp; relocate", zipfile.physical);
        for (size_t i = 0; i < nmaps; i++) {
            printf(" %s=>%s", maps[i].physical, maps[i].leaf);
        }
        fputc('\n', stdout);
        rc = 0;
        goto out;
    }

    /* unzip -d tmpdir zipfile members... */
    {
        ArgList argv = {0};
        const char *ucmd = wrap_is_rewrite() ? desc->real_cmd : wrap_exec_name(desc);
        arglist_push(&argv, xstrdup(ucmd));
        for (size_t i = 0; i < opts->n; i++) {
            arglist_push(&argv, xstrdup(opts->v[i]));
        }
        arglist_push(&argv, xstrdup("-d"));
        arglist_push(&argv, xstrdup(tmpdir));
        arglist_push(&argv, xstrdup(zipfile.physical));
        for (size_t i = 0; i < members.n; i++) {
            arglist_push(&argv, xstrdup(members.v[i]));
        }

        if (wrap_is_rewrite()) {
            if (!desc->rewrite) {
                arglist_free(&argv, 1);
                goto out;
            }
            int r = desc->rewrite((int)argv.n, argv.v);
            arglist_free(&argv, 1);
            if (r > 1) {
                rc = r;
                goto out;
            }
        } else {
            pid_t pid = fork();
            if (pid < 0) {
                arglist_free(&argv, 1);
                goto out;
            }
            if (pid == 0) {
                execvp(ucmd, argv.v);
                _exit(127);
            }
            int st = 0;
            waitpid(pid, &st, 0);
            arglist_free(&argv, 1);
            /* unzip: 0 ok, 1 warnings, 11 no matches for some patterns */
            if (!WIFEXITED(st) || (WEXITSTATUS(st) > 1 && WEXITSTATUS(st) != 11)) {
                rc = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
                goto out;
            }
        }
    }

    for (size_t i = 0; i < nmaps; i++) {
        char *from = colon_join(tmpdir, maps[i].physical);
        if (!from) {
            goto out;
        }
        struct stat stfrom;
        if (stat(from, &stfrom) != 0) {
            fprintf(stderr, "%s: nothing extracted for %s\n", desc->prog, maps[i].physical);
            free(from);
            goto out;
        }
        if (colon_mkdir_parents(maps[i].leaf) != 0) {
            fprintf(stderr, "%s: %s: %s\n", desc->prog, maps[i].leaf, strerror(errno));
            free(from);
            goto out;
        }
        rm_rf(maps[i].leaf);
        if (run_mv(from, maps[i].leaf) != 0) {
            free(from);
            goto out;
        }
        free(from);
    }
    rc = 0;

out:
    for (size_t i = 0; i < nmaps; i++) {
        free(maps[i].physical);
        free(maps[i].leaf);
    }
    free(maps);
    arglist_free(&members, 1);
    rm_rf(tmpdir);
    free(tmpdir);
    colon_path_free(&zipfile);
    return rc;
}

int colon_cmd_main(const ColonCmdDesc *desc, int argc, char **argv) {
    g_argv0 = (argv && argv[0]) ? argv[0] : desc->prog;
    init_i18n(LOCALEDIR);

    ArgList opts = {0};
    ArgList files = {0};
    int saw_help = 0, saw_version = 0, dry_run = 0;

    if (split_argv(desc, argc, argv, &opts, &files, &saw_help, &saw_version, &dry_run) != 0) {
        fprintf(stderr, "%s: out of memory\n", desc->prog);
        return 1;
    }

    if (saw_help) {
        print_help(desc);
        arglist_free(&opts, 1);
        arglist_free(&files, 1);
        return 0;
    }
    if (saw_version) {
        print_version(desc);
        arglist_free(&opts, 1);
        arglist_free(&files, 1);
        return 0;
    }

    int rc = 1;

    switch (desc->style) {
    case COLON_STYLE_SOURCES: {
        ArgList out = {0};
        if (rewrite_sources(&files, &out) != 0) {
            break;
        }
        rc = run_exec(desc, &opts, out.v, dry_run);
        arglist_free(&out, 1);
        break;
    }
    case COLON_STYLE_RM:
        rc = do_rm_or_rmdir(desc, &opts, &files, dry_run, 0);
        break;
    case COLON_STYLE_RMDIR:
        rc = do_rm_or_rmdir(desc, &opts, &files, dry_run, 1);
        break;
    case COLON_STYLE_SRCDEST: {
        ArgList pairs = {0};
        int np = build_srcdest_pairs(desc, &opts, &files, &pairs);
        if (np < 0) {
            break;
        }
        if (np == 0 && pairs.n == 0) {
            rc = run_exec(desc, &opts, NULL, dry_run);
        } else if (np <= 1 && pairs.n == 2) {
            /* single pair — one exec */
            rc = run_exec(desc, &opts, pairs.v, dry_run);
        } else if (pairs.n >= 2 && pairs.n % 2 == 0) {
            rc = 0;
            for (size_t i = 0; i + 1 < pairs.n; i += 2) {
                int r = run_exec_pair(desc, &opts, pairs.v[i], pairs.v[i + 1], dry_run);
                if (r != 0) {
                    rc = r;
                    break;
                }
            }
        } else {
            rc = run_exec(desc, &opts, pairs.v, dry_run);
        }
        arglist_free(&pairs, 1);
        break;
    }
    case COLON_STYLE_ZIP:
        rc = do_zip(desc, &opts, &files, dry_run);
        break;
    case COLON_STYLE_UNZIP:
        rc = do_unzip(desc, &opts, &files, dry_run);
        break;
    }

    arglist_free(&opts, 1);
    arglist_free(&files, 1);
    return rc;
}
