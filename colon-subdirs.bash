# bash completion for colon-subdirs (:mv / colon-mv, …)
# Completes BASE:LEAF by listing names under BASE/ (colon stays in the word).

# shellcheck shell=bash

_colon_subdirs_init() {
    # Keep ':' inside $cur (same idea as scp/rsync completions).
    if declare -F _comp_initialize >/dev/null 2>&1; then
        _comp_initialize -n : -- "$@" || return 1
        return 0
    fi
    if declare -F _get_comp_words_by_ref >/dev/null 2>&1; then
        _get_comp_words_by_ref -n : cur prev words cword || return 1
        return 0
    fi
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD - 1]-}"
    return 0
}

# host:path for scp/rsync — BASE has no '/', and is not . / ..
_colon_subdirs_is_host_path() {
    local cur=$1 base
    [[ $cur == *:* && $cur != *::* && $cur != *://* ]] || return 1
    base=${cur%%:*}
    [[ $base != */* && $base != . && $base != .. ]]
}

_colon_subdirs_filedir() {
    local cur=$1 p
    local -a raw=()

    # compgen returns 1 when there are no matches — do not fail under set -e
    # shellcheck disable=SC2207
    raw=($(compgen -f -- "$cur" || true))
    COMPREPLY=()
    for p in "${raw[@]}"; do
        if [[ -d $p && $p != */ ]]; then
            COMPREPLY+=("$p/")
        else
            COMPREPLY+=("$p")
        fi
    done
    compopt -o nospace 2>/dev/null || true
}

# Complete CUR as BASE:LEAF → suggestions keep the colon-path form.
_colon_subdirs_colonpath() {
    local cur=$1
    local base leaf search_dir leaf_parent leaf_prefix
    local f name reply
    local -a names
    local nullglob_was=0

    base=${cur%%:*}
    leaf=${cur#*:}
    leaf=${leaf#/}

    search_dir=$base
    leaf_parent=
    leaf_prefix=$leaf

    if [[ $leaf == */ ]]; then
        leaf_parent=${leaf%/}
        leaf_prefix=
        search_dir=$base/$leaf_parent
    elif [[ $leaf == */* ]]; then
        leaf_parent=${leaf%/*}
        leaf_prefix=${leaf##*/}
        search_dir=$base/$leaf_parent
    fi

    COMPREPLY=()
    [[ -d $search_dir ]] || return 0

    shopt -q nullglob && nullglob_was=1
    shopt -s nullglob
    if [[ -n $leaf_prefix ]]; then
        names=("$search_dir"/"$leaf_prefix"*)
    else
        names=("$search_dir"/*)
    fi
    ((nullglob_was)) || shopt -u nullglob

    for f in "${names[@]}"; do
        [[ -e $f || -L $f ]] || continue
        name=${f#"$search_dir"/}
        [[ $name == . || $name == .. ]] && continue
        if [[ -n $leaf_parent ]]; then
            reply="${base}:${leaf_parent}/${name}"
        else
            reply="${base}:${name}"
        fi
        if [[ -d $f ]]; then
            reply+=/
        fi
        COMPREPLY+=("$reply")
    done

    # filenames option mangles colon-paths; nospace keeps trailing / on dirs
    compopt -o nospace 2>/dev/null || true
}

# $1=cur  $2=remote_aware(0|1)
_colon_subdirs_complete_arg() {
    local cur=$1
    local remote_aware=${2:-0}

    case $cur in
        -*)
            # shellcheck disable=SC2207
            COMPREPLY=($(compgen -W '--help --version --colon-dry-run -h' -- "$cur" || true))
            return 0
            ;;
        *://* | *::*)
            _colon_subdirs_filedir "$cur"
            return 0
            ;;
        *:*)
            if ((remote_aware)) && _colon_subdirs_is_host_path "$cur"; then
                _colon_subdirs_filedir "$cur"
                return 0
            fi
            _colon_subdirs_colonpath "$cur"
            return 0
            ;;
        *)
            _colon_subdirs_filedir "$cur"
            return 0
            ;;
    esac
}

_colon_subdirs_cmd() {
    local cur prev words cword
    local remote_aware=0
    local cmd=${1-}

    case $cmd in
        colon-scp | :scp | colon-rsync | :rsync) remote_aware=1 ;;
    esac

    _colon_subdirs_init "$@" || return

    # Fallback if init did not set cur
    [[ -n ${cur-} ]] || cur="${COMP_WORDS[COMP_CWORD]}"

    _colon_subdirs_complete_arg "$cur" "$remote_aware"
}

_colon_subdirs_register() {
    local cmd
    for cmd in mv cp zip rm rmdir scp unzip rsync; do
        complete -F _colon_subdirs_cmd -o nospace "colon-${cmd}" ":${cmd}"
    done
}

_colon_subdirs_register
