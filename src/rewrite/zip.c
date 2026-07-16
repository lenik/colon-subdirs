/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Minimal store-only zip writer (no compression). Supports -r for directories.
 */

#define _DEFAULT_SOURCE

#include "rewrite.h"
#include "rewrite/util.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    char *name;
    uint32_t crc;
    uint32_t size;
    uint32_t offset;
    uint16_t method;
} ZipEnt;

typedef struct {
    ZipEnt *v;
    size_t n;
    size_t cap;
} ZipList;

static uint32_t crc_table[256];
static int crc_init_done;

static void crc_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) {
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        crc_table[i] = c;
    }
    crc_init_done = 1;
}

static uint32_t crc32_update(uint32_t crc, const void *buf, size_t n) {
    if (!crc_init_done) {
        crc_init();
    }
    const unsigned char *p = buf;
    crc = ~crc;
    for (size_t i = 0; i < n; i++) {
        crc = crc_table[(crc ^ p[i]) & 0xff] ^ (crc >> 8);
    }
    return ~crc;
}

static void put_u16(FILE *f, uint16_t v) {
    fputc(v & 0xff, f);
    fputc((v >> 8) & 0xff, f);
}

static void put_u32(FILE *f, uint32_t v) {
    fputc(v & 0xff, f);
    fputc((v >> 8) & 0xff, f);
    fputc((v >> 16) & 0xff, f);
    fputc((v >> 24) & 0xff, f);
}

static int add_ent(ZipList *z, const char *name, uint32_t crc, uint32_t size, uint32_t off) {
    if (z->n + 1 >= z->cap) {
        size_t nc = z->cap ? z->cap * 2 : 16;
        ZipEnt *nv = realloc(z->v, nc * sizeof(ZipEnt));
        if (!nv) {
            return -1;
        }
        z->v = nv;
        z->cap = nc;
    }
    z->v[z->n].name = strdup(name);
    if (!z->v[z->n].name) {
        return -1;
    }
    z->v[z->n].crc = crc;
    z->v[z->n].size = size;
    z->v[z->n].offset = off;
    z->v[z->n].method = 0;
    z->n++;
    return 0;
}

static int write_file(FILE *out, ZipList *z, const char *arcname, const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return -1;
    }
    uint32_t crc = 0, size = 0;
    char buf[8192];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n < 0) {
            close(fd);
            return -1;
        }
        if (n == 0) {
            break;
        }
        crc = crc32_update(crc, buf, (size_t)n);
        size += (uint32_t)n;
    }
    lseek(fd, 0, SEEK_SET);

    uint32_t off = (uint32_t)ftell(out);
    put_u32(out, 0x04034b50u);
    put_u16(out, 20);
    put_u16(out, 0);
    put_u16(out, 0);
    put_u16(out, 0);
    put_u16(out, 0);
    put_u32(out, crc);
    put_u32(out, size);
    put_u32(out, size);
    put_u16(out, (uint16_t)strlen(arcname));
    put_u16(out, 0);
    fwrite(arcname, 1, strlen(arcname), out);
    for (;;) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n <= 0) {
            break;
        }
        fwrite(buf, 1, (size_t)n, out);
    }
    close(fd);
    (void)st;
    return add_ent(z, arcname, crc, size, off);
}

static int write_tree(FILE *out, ZipList *z, const char *arcprefix, const char *path, int recursive) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        if (!recursive) {
            fprintf(stderr, "zip: %s is a directory (use -r)\n", path);
            return -1;
        }
        char name[4096];
        snprintf(name, sizeof name, "%s%s%s", arcprefix, arcprefix[0] ? "/" : "", "");
        /* store directory entry */
        char dname[4096];
        if (arcprefix[0]) {
            snprintf(dname, sizeof dname, "%s/", arcprefix);
        } else {
            const char *base = strrchr(path, '/');
            base = base ? base + 1 : path;
            snprintf(dname, sizeof dname, "%s/", base);
        }
        uint32_t off = (uint32_t)ftell(out);
        put_u32(out, 0x04034b50u);
        put_u16(out, 20);
        put_u16(out, 0);
        put_u16(out, 0);
        put_u16(out, 0);
        put_u16(out, 0);
        put_u32(out, 0);
        put_u32(out, 0);
        put_u32(out, 0);
        put_u16(out, (uint16_t)strlen(dname));
        put_u16(out, 0);
        fwrite(dname, 1, strlen(dname), out);
        if (add_ent(z, dname, 0, 0, off) != 0) {
            return -1;
        }

        DIR *d = opendir(path);
        if (!d) {
            return -1;
        }
        struct dirent *de;
        int rc = 0;
        while ((de = readdir(d)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
                continue;
            }
            char child[4096], child_arc[4096];
            snprintf(child, sizeof child, "%s/%s", path, de->d_name);
            if (arcprefix[0]) {
                snprintf(child_arc, sizeof child_arc, "%s/%s", arcprefix, de->d_name);
            } else {
                const char *base = strrchr(path, '/');
                base = base ? base + 1 : path;
                snprintf(child_arc, sizeof child_arc, "%s/%s", base, de->d_name);
            }
            if (write_tree(out, z, child_arc, child, 1) != 0) {
                rc = -1;
                break;
            }
        }
        closedir(d);
        return rc;
    }
    return write_file(out, z, arcprefix, path);
}

int rewrite_zip(int argc, char **argv) {
    const char *prog = argv[0] ? argv[0] : "zip";
    int recursive = 0;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][1]) {
        if (strcmp(argv[i], "--") == 0) {
            i++;
            break;
        }
        for (char *p = argv[i] + 1; *p; p++) {
            if (*p == 'r') {
                recursive = 1;
            }
        }
        i++;
    }
    if (i >= argc) {
        fprintf(stderr, "%s: missing zipfile\n", prog);
        return 1;
    }
    const char *zipfile = argv[i++];
    if (i >= argc) {
        fprintf(stderr, "%s: nothing to zip\n", prog);
        return 1;
    }

    char *parent = strdup(zipfile);
    if (parent) {
        char *slash = strrchr(parent, '/');
        if (slash && slash != parent) {
            *slash = '\0';
            rw_mkdir_p(parent);
        }
        free(parent);
    }

    FILE *out = fopen(zipfile, "wb");
    if (!out) {
        rw_perror(prog, zipfile);
        return 1;
    }

    ZipList z = {0};
    int rc = 0;
    for (; i < argc; i++) {
        const char *path = argv[i];
        const char *base = strrchr(path, '/');
        base = base ? base + 1 : path;
        if (write_tree(out, &z, base, path, recursive) != 0) {
            rw_perror(prog, path);
            rc = 1;
            break;
        }
    }

    uint32_t cd_off = (uint32_t)ftell(out);
    for (size_t k = 0; k < z.n; k++) {
        ZipEnt *e = &z.v[k];
        put_u32(out, 0x02014b50u);
        put_u16(out, 20);
        put_u16(out, 20);
        put_u16(out, 0);
        put_u16(out, e->method);
        put_u16(out, 0);
        put_u16(out, 0);
        put_u32(out, e->crc);
        put_u32(out, e->size);
        put_u32(out, e->size);
        put_u16(out, (uint16_t)strlen(e->name));
        put_u16(out, 0);
        put_u16(out, 0);
        put_u16(out, 0);
        put_u16(out, 0);
        put_u32(out, 0);
        put_u32(out, e->offset);
        fwrite(e->name, 1, strlen(e->name), out);
    }
    uint32_t cd_size = (uint32_t)ftell(out) - cd_off;
    put_u32(out, 0x06054b50u);
    put_u16(out, 0);
    put_u16(out, 0);
    put_u16(out, (uint16_t)z.n);
    put_u16(out, (uint16_t)z.n);
    put_u32(out, cd_size);
    put_u32(out, cd_off);
    put_u16(out, 0);

    for (size_t k = 0; k < z.n; k++) {
        free(z.v[k].name);
    }
    free(z.v);
    if (fclose(out) != 0) {
        rc = 1;
    }
    return rc;
}
