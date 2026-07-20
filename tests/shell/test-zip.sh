#!/usr/bin/env bash
# Integration test for colon-zip and colon-unzip.
# shellcheck shell=bash
source "$(dirname "$0")/common.sh"
setup_tmp

ZIP=$(bin zip)
UNZIP=$(bin unzip)

mkdir -p base/c/d/e
echo nested >base/c/d/e/x.txt
echo root >base/c/readme.txt

"$ZIP" -r archive.zip 'base:c'
assert_file archive.zip
pass "zip created archive.zip"

# Extract physical-layout members with colon mapping
mkdir -p staging/a/b/c/d/e
echo fromzip >staging/a/b/c/d/e/y.txt
(cd staging && zip -rq ../phys.zip a/b/c/d/e)

"$UNZIP" phys.zip 'a/b:/c/d/e'
assert_file c/d/e/y.txt
assert_content c/d/e/y.txt fromzip
pass "unzip a/b:/c/d/e extracts to c/d/e/"

# archive from colon-zip should contain leaf path c/...
if command -v unzip >/dev/null; then
    listing=$(unzip -Z1 archive.zip 2>/dev/null || unzip -l archive.zip)
    if ! grep -qE '(^|/)c/(d/e/x\.txt|readme\.txt)' <<<"$listing"; then
        echo "archive.zip listing:" >&2
        printf '%s\n' "$listing" >&2
        exit 1
    fi
    pass "archive.zip stores leaf names under c/"
fi

echo "ALL PASS: colon-zip / colon-unzip"
