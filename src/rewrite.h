/*
 * Copyright (C) 2026 Lenik <lenik@bodz.net>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef COLON_REWRITE_H
#define COLON_REWRITE_H

/*
 * Built-in functional equivalents of the wrapped tools.
 * Invoked when COLON_WRAP is unset (default). Arguments are already
 * colon-path-rewritten (physical paths) unless noted by the caller.
 *
 * argv[0] is the command name; remaining args match the external tool.
 */
int rewrite_mv(int argc, char **argv);
int rewrite_cp(int argc, char **argv);
int rewrite_rm(int argc, char **argv);
int rewrite_rmdir(int argc, char **argv);
int rewrite_zip(int argc, char **argv);
int rewrite_unzip(int argc, char **argv);
int rewrite_scp(int argc, char **argv);
int rewrite_rsync(int argc, char **argv);

typedef int (*ColonRewriteFn)(int argc, char **argv);

#endif /* COLON_REWRITE_H */
