#!/usr/bin/env bash
# Integration test for colon-rm and colon-rmdir (leaf parent prune).
# shellcheck shell=bash
source "$(dirname "$0")/common.sh"
setup_tmp

RM=$(bin rm)
RMDIR=$(bin rmdir)

# --- rm file then prune empty leaf parents ---
mkdir -p nest/d/e
echo f >nest/d/e/f
"$RM" '.:nest/d/e/f'
assert_missing nest/d/e/f
assert_missing nest/d/e
assert_missing nest/d
assert_missing nest
pass "rm .:nest/d/e/f prunes empty leaf dirs"

# --- rmdir empty leaf chain, keep BASE ---
mkdir -p keep/d/e/f
"$RMDIR" 'keep:d/e/f'
assert_missing keep/d
assert_dir keep
pass "rmdir keep:d/e/f removes empty leaf, keeps BASE"

# --- rmdir stops at non-empty parent ---
mkdir -p keep2/d/e/f
echo stay >keep2/d/e/keep.txt
"$RMDIR" 'keep2:d/e/f'
assert_missing keep2/d/e/f
assert_file keep2/d/e/keep.txt
assert_dir keep2/d/e
pass "rmdir stops when parent not empty"

echo "ALL PASS: colon-rm / colon-rmdir"
