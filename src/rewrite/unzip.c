/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Minimal store-only unzip (compression method 0).
 */

#define _DEFAULT_SOURCE

#include "rewrite.h"
#include "rewrite/util.h"

#include <fcntl.h>
#include <fnmatch.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static uint16_t get_u16(const unsigned char *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t get_u32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int name_selected(const char *name, char **members, int nmem) {
    if (nmem == 0) {
        return 1;
    }
    for (int i = 0; i < nmem; i++) {
        const char *pat = members[i];
        if (strcmp(name, pat) == 0) {
            return 1;
        }
        size_t plen = strlen(pat);
        if (plen > 0 && strncmp(name, pat, plen) == 0 &&
            (name[plen] == '/' || name[plen] == '\0')) {
            return 1;
        }
        if (fnmatch(pat, name, FNM_PATHNAME) == 0) {
            return 1;
        }
    }
    return 0;
}

static int unsafe_zip_path(const char *name) {
    if (!name || !*name) {
        return 1;
    }
    if (name[0] == '/') {
        return 1;
    }
    if (strncmp(name, "../", 3) == 0) {
        return 1;
    }
    for (const char *p = name; *p; p++) {
        if (p[0] == '/' && p[1] == '.' && p[2] == '.' &&
            (p[3] == '/' || p[3] == '\0')) {
            return 1;
        }
    }
    return 0;
}

static int join_dest(char *out, size_t outsz, const char *exdir, const char *name) {
    int n = snprintf(out, outsz, "%s/%s", exdir, name);
    return n > 0 && (size_t)n < outsz ? 0 : -1;
}

static int extract_stored(const char *prog, FILE *zf, const char *exdir, const char *name,
                          uint32_t comp_size, uint32_t uncomp_size, uint32_t lfh_off) {
    if (unsafe_zip_path(name)) {
        fprintf(stderr, "%s: skipping unsafe path '%s'\n", prog, name);
        return 1;
    }

    size_t nlen = strlen(name);
    int is_dir = (nlen > 0 && name[nlen - 1] == '/');

    char path[4096];
    if (join_dest(path, sizeof path, exdir, name) != 0) {
        fprintf(stderr, "%s: path too long: %s\n", prog, name);
        return 1;
    }

    if (is_dir) {
        size_t plen = strlen(path);
        if (plen > 0 && path[plen - 1] == '/') {
            path[plen - 1] = '\0';
        }
        if (path[0] && rw_mkdir_p(path) != 0) {
            rw_perror(prog, path);
            return 1;
        }
        return 0;
    }

    char *parent = strdup(path);
    if (parent) {
        char *slash = strrchr(parent, '/');
        if (slash && slash != parent) {
            *slash = '\0';
            if (rw_mkdir_p(parent) != 0) {
                rw_perror(prog, parent);
                free(parent);
                return 1;
            }
        }
        free(parent);
    }

    if (fseek(zf, (long)lfh_off, SEEK_SET) != 0) {
        rw_perror(prog, "seek local header");
        return 1;
    }
    unsigned char lfh[30];
    if (fread(lfh, 1, sizeof lfh, zf) != sizeof lfh) {
        fprintf(stderr, "%s: corrupt local header for '%s'\n", prog, name);
        return 1;
    }
    if (get_u32(lfh) != 0x04034b50u) {
        fprintf(stderr, "%s: bad local header for '%s'\n", prog, name);
        return 1;
    }
    uint16_t name_len = get_u16(lfh + 26);
    uint16_t extra_len = get_u16(lfh + 28);
    if (fseek(zf, (long)(name_len + extra_len), SEEK_CUR) != 0) {
        return 1;
    }

    if (comp_size != uncomp_size) {
        fprintf(stderr, "%s: size mismatch for '%s'\n", prog, name);
        return 1;
    }

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        rw_perror(prog, path);
        return 1;
    }

    unsigned char buf[8192];
    uint32_t left = comp_size;
    int rc = 0;
    while (left > 0) {
        size_t chunk = left > sizeof buf ? sizeof buf : left;
        size_t n = fread(buf, 1, chunk, zf);
        if (n != chunk) {
            fprintf(stderr, "%s: short read for '%s'\n", prog, name);
            rc = 1;
            break;
        }
        char *p = (char *)buf;
        size_t rem = n;
        while (rem > 0) {
            ssize_t w = write(fd, p, rem);
            if (w < 0) {
                rw_perror(prog, path);
                rc = 1;
                rem = 0;
                break;
            }
            p += w;
            rem -= (size_t)w;
        }
        left -= (uint32_t)n;
    }
    if (close(fd) != 0 && rc == 0) {
        rw_perror(prog, path);
        rc = 1;
    }
    return rc;
}

static int find_eocd(FILE *zf, long *eocd_off) {
    if (fseek(zf, 0, SEEK_END) != 0) {
        return -1;
    }
    long sz = ftell(zf);
    if (sz < 22) {
        return -1;
    }
    long scan = sz > 65557 ? 65557 : sz;
    unsigned char *buf = malloc((size_t)scan);
    if (!buf) {
        return -1;
    }
    if (fseek(zf, sz - scan, SEEK_SET) != 0) {
        free(buf);
        return -1;
    }
    if (fread(buf, 1, (size_t)scan, zf) != (size_t)scan) {
        free(buf);
        return -1;
    }
    for (long i = scan - 22; i >= 0; i--) {
        if (get_u32(buf + i) == 0x06054b50u) {
            *eocd_off = sz - scan + i;
            free(buf);
            return 0;
        }
    }
    free(buf);
    return -1;
}

int rewrite_unzip(int argc, char **argv) {
    const char *prog = argv[0] ? argv[0] : "unzip";
    const char *exdir = ".";
    const char *zipfile = NULL;
    char **members = NULL;
    int nmem = 0;
    int memcap = 0;

    int i = 1;
    while (i < argc) {
        const char *a = argv[i];
        if (strcmp(a, "-d") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "%s: -d requires an argument\n", prog);
                free(members);
                return 1;
            }
            exdir = argv[++i];
            i++;
            continue;
        }
        if (strncmp(a, "-d", 2) == 0 && a[2]) {
            exdir = a + 2;
            i++;
            continue;
        }
        if (strcmp(a, "--") == 0) {
            i++;
            while (i < argc) {
                if (!zipfile) {
                    zipfile = argv[i++];
                } else {
                    if (nmem + 1 >= memcap) {
                        int nc = memcap ? memcap * 2 : 8;
                        char **nv = realloc(members, (size_t)nc * sizeof(char *));
                        if (!nv) {
                            free(members);
                            return 1;
                        }
                        members = nv;
                        memcap = nc;
                    }
                    members[nmem++] = argv[i++];
                }
            }
            break;
        }
        if (a[0] == '-' && a[1]) {
            i++;
            continue;
        }
        if (!zipfile) {
            zipfile = a;
        } else {
            if (nmem + 1 >= memcap) {
                int nc = memcap ? memcap * 2 : 8;
                char **nv = realloc(members, (size_t)nc * sizeof(char *));
                if (!nv) {
                    free(members);
                    return 1;
                }
                members = nv;
                memcap = nc;
            }
            members[nmem++] = a;
        }
        i++;
    }

    if (!zipfile) {
        fprintf(stderr, "%s: missing zipfile\n", prog);
        free(members);
        return 1;
    }

    if (rw_mkdir_p(exdir) != 0) {
        rw_perror(prog, exdir);
        free(members);
        return 1;
    }

    FILE *zf = fopen(zipfile, "rb");
    if (!zf) {
        rw_perror(prog, zipfile);
        free(members);
        return 1;
    }

    long eocd = 0;
    if (find_eocd(zf, &eocd) != 0) {
        fprintf(stderr, "%s: cannot find end of central directory in '%s'\n", prog, zipfile);
        fclose(zf);
        free(members);
        return 1;
    }

    unsigned char eocd_buf[22];
    if (fseek(zf, eocd, SEEK_SET) != 0 || fread(eocd_buf, 1, sizeof eocd_buf, zf) != sizeof eocd_buf) {
        fprintf(stderr, "%s: corrupt zip '%s'\n", prog, zipfile);
        fclose(zf);
        free(members);
        return 1;
    }

    uint16_t total = get_u16(eocd_buf + 10);
    uint32_t cd_size = get_u32(eocd_buf + 12);
    uint32_t cd_off = get_u32(eocd_buf + 16);

    if (fseek(zf, (long)cd_off, SEEK_SET) != 0) {
        rw_perror(prog, "central directory");
        fclose(zf);
        free(members);
        return 1;
    }

    int rc = 0;
    for (uint16_t ent = 0; ent < total; ent++) {
        unsigned char hdr[46];
        if (fread(hdr, 1, sizeof hdr, zf) != sizeof hdr) {
            fprintf(stderr, "%s: corrupt central directory\n", prog);
            rc = 1;
            break;
        }
        if (get_u32(hdr) != 0x02014b50u) {
            fprintf(stderr, "%s: bad central directory entry\n", prog);
            rc = 1;
            break;
        }
        uint16_t method = get_u16(hdr + 10);
        uint32_t comp_size = get_u32(hdr + 20);
        uint32_t uncomp_size = get_u32(hdr + 24);
        uint16_t name_len = get_u16(hdr + 28);
        uint16_t extra_len = get_u16(hdr + 30);
        uint16_t comment_len = get_u16(hdr + 32);
        uint32_t lfh_off = get_u32(hdr + 42);

        char *name = malloc((size_t)name_len + 1);
        if (!name) {
            rc = 1;
            break;
        }
        if (fread(name, 1, name_len, zf) != name_len) {
            free(name);
            rc = 1;
            break;
        }
        name[name_len] = '\0';
        if (fseek(zf, (long)(extra_len + comment_len), SEEK_CUR) != 0) {
            free(name);
            rc = 1;
            break;
        }

        if (!name_selected(name, members, nmem)) {
            free(name);
            continue;
        }

        if (method != 0) {
            fprintf(stderr, "%s: skipping '%s': unsupported compression method %u\n", prog, name,
                    (unsigned)method);
            free(name);
            continue;
        }

        if (extract_stored(prog, zf, exdir, name, comp_size, uncomp_size, lfh_off) != 0) {
            rc = 1;
        }
        free(name);
    }

    (void)cd_size;
    fclose(zf);
    free(members);
    return rc;
}
