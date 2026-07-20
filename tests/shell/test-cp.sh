#!/usr/bin/env bash
# Integration test for colon-cp.
# shellcheck shell=bash
source "$(dirname "$0")/common.sh"
setup_tmp

CP=$(bin cp)

mkdir -p tree/sub/deep
echo payload >tree/sub/deep/a.txt
echo other >tree/sub/b.txt

"$CP" -r 'tree:sub' copy_dest/
assert_file copy_dest/sub/deep/a.txt
assert_content copy_dest/sub/deep/a.txt payload
assert_file copy_dest/sub/b.txt
# source must still exist
assert_file tree/sub/deep/a.txt
pass "cp -r BASE:LEAF into directory"

"$CP" -T 'tree:sub/b.txt' alone.txt
assert_file alone.txt
assert_content alone.txt other
pass "cp -T single file to new name"

echo "ALL PASS: colon-cp"
