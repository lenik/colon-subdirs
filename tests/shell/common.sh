#!/usr/bin/env bash
# Shared helpers for tests/shell/test-*.sh
# shellcheck shell=bash

set -euo pipefail

SHELL_TEST_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(cd "$SHELL_TEST_DIR/../.." && pwd)

# Binaries: COLON_BINDIR (meson), else /build, else $ROOT/build
if [[ -z "${COLON_BINDIR:-}" ]]; then
    if [[ -x /build/colon-mv ]]; then
        COLON_BINDIR=/build
    elif [[ -x $ROOT/build/colon-mv ]]; then
        COLON_BINDIR=$ROOT/build
    else
        echo "COLON_BINDIR not set and no built colon-mv found" >&2
        exit 1
    fi
fi

bin() {
    local name=$1
    local p=$COLON_BINDIR/colon-$name
    [[ -x $p ]] || {
        echo "missing executable: $p" >&2
        exit 1
    }
    printf '%s' "$p"
}

# Fresh workspace under $TMP (caller sets trap)
setup_tmp() {
    TMP=$(mktemp -d)
    export TMP
    # shellcheck disable=SC2064
    trap 'rm -rf "$TMP"' EXIT
    cd "$TMP"
}

assert_file() {
    local path=$1
    [[ -f $path ]] || {
        echo "FAIL: expected file: $path" >&2
        ls -laR "$(dirname "$path")" 2>/dev/null || true
        exit 1
    }
}

assert_dir() {
    local path=$1
    [[ -d $path ]] || {
        echo "FAIL: expected directory: $path" >&2
        exit 1
    }
}

assert_missing() {
    local path=$1
    [[ ! -e $path ]] || {
        echo "FAIL: expected missing: $path" >&2
        exit 1
    }
}

assert_content() {
    local path=$1
    local expect=$2
    local got
    got=$(cat -- "$path")
    [[ $got == "$expect" ]] || {
        echo "FAIL: $path content '$got' != '$expect'" >&2
        exit 1
    }
}

pass() {
    echo "OK: $*"
}
