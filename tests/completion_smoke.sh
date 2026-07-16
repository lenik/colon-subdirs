#!/usr/bin/env bash
# Non-interactive smoke test for colon-path bash completion.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
# shellcheck source=../colon-subdirs.bash
source "$ROOT/colon-subdirs.bash"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/a/b/c/d" "$TMP/a/b/c/other"
echo x >"$TMP/a/b/c/d/file.txt"
echo y >"$TMP/a/b/c/foo.txt"
cd "$TMP"

# Simulate completion of a/b:c/
COMP_WORDS=(colon-mv "a/b:c/")
COMP_CWORD=1
COMP_LINE='colon-mv a/b:c/'
COMP_POINT=${#COMP_LINE}
COMPREPLY=()
_colon_subdirs_cmd colon-mv "a/b:c/" "colon-mv"

echo "completions for a/b:c/:"
printf '  %q\n' "${COMPREPLY[@]}"

found_d=0 found_foo=0
for r in "${COMPREPLY[@]}"; do
    [[ $r == a/b:c/d/ ]] && found_d=1
    [[ $r == a/b:c/foo.txt ]] && found_foo=1
done
((found_d)) || { echo "missing a/b:c/d/"; exit 1; }
((found_foo)) || { echo "missing a/b:c/foo.txt"; exit 1; }

# Nested leaf prefix
COMPREPLY=()
_colon_subdirs_complete_arg "a/b:c/d/" 0
echo "completions for a/b:c/d/:"
printf '  %q\n' "${COMPREPLY[@]}"
found_file=0
for r in "${COMPREPLY[@]}"; do
    [[ $r == a/b:c/d/file.txt ]] && found_file=1
done
((found_file)) || { echo "missing a/b:c/d/file.txt"; exit 1; }

# host:path must not be rewritten for scp
COMPREPLY=()
_colon_subdirs_complete_arg "host:path" 1
echo "scp host:path replies=${#COMPREPLY[@]} (filedir fallback ok)"

echo OK
