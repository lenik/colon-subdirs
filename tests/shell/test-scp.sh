#!/usr/bin/env bash
# Integration test for local colon-scp / colon-rsync (rewrite, no remote).
# Bases must contain '/' so paths are colon-paths, not host:path.
# shellcheck shell=bash
source "$(dirname "$0")/common.sh"
setup_tmp

SCP=$(bin scp)
RSYNC=$(bin rsync)

mkdir -p from/x/leaf
echo scp-data >from/x/leaf/f.txt

"$SCP" 'from/x:leaf' scp_out/
assert_file scp_out/leaf/f.txt
assert_content scp_out/leaf/f.txt scp-data
pass "scp local BASE:LEAF"

mkdir -p sync_src/x/data
echo rsync-data >sync_src/x/data/r.txt
"$RSYNC" -a 'sync_src/x:data' sync_dst/
assert_file sync_dst/data/r.txt
assert_content sync_dst/data/r.txt rsync-data
pass "rsync local BASE:LEAF"

# Remote forms must fail under rewrite (default). `if` avoids set -e abort.
if "$SCP" 'host:path' /tmp/ 2>/dev/null; then
    echo "FAIL: scp should reject host:path under rewrite" >&2
    exit 1
fi
pass "scp rejects host:path"

echo "ALL PASS: colon-scp / colon-rsync"
