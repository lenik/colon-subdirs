#!/usr/bin/env bash
# Integration test for colon-mv (built-in rewrite by default).
# shellcheck shell=bash
source "$(dirname "$0")/common.sh"
setup_tmp

MV=$(bin mv)

# --- move colon-path into directory (DEST/LEAF) ---
mkdir -p srcbase/leaf/dir
echo hello >srcbase/leaf/dir/file.txt
"$MV" 'srcbase:leaf/dir' dest/
assert_file dest/leaf/dir/file.txt
assert_content dest/leaf/dir/file.txt hello
assert_missing srcbase/leaf/dir/file.txt
pass "mv BASE:LEAF into dest/"

# --- exact file rename with -T (no DEST/LEAF) ---
mkdir -p a/b/c
echo x >a/b/c/item
"$MV" -T 'a/b:c/item' 'renamed.txt'
assert_file renamed.txt
assert_content renamed.txt x
assert_missing a/b/c/item
pass "mv -T exact rename to file"

# --- into directory preserves leaf ---
mkdir -p pkg/data
echo y >pkg/data/z
"$MV" 'pkg:data/z' outdir/
assert_file outdir/data/z
assert_content outdir/data/z y
pass "mv pkg:data/z into outdir/"

echo "ALL PASS: colon-mv"
