#!/bin/sh

set -eu

usage () {
    printf 'Usage: %s NAME=VAL ...+ [FILE|DIRECTORY] ...\n' "$0"
    exit ${1:-0}
}

main () {
    ME="${0##*/}"
    warn_count=0

    todo="$(process_arguments "$@")"
    eval "$todo"

    local src
    for src; do
        process_input_arg "$src"
    done

    show_warn_count
}

process_input_arg () {
    if [ -d "$1" ]; then
        process_dir "$1"
    elif [ -e "$1" ]; then
        process_file_arg "$1"
    else
        skip_warning "$1" 'File does not exist'
    fi
}

process_file_arg () {
    set -- "$1" "${1%.in}"
    if [ "$1" = "$2" ]; then
        skip_warning "$1" 'Filename should end with ‘.in’'
    else
        process_file "$1" "$2"
    fi
}

process_dir () {
    dir_is_nonempty "$1" || return 0

    local src
    local dst

    for src in "$1"/*; do
        dst="${src%.in}"
        if [ -d "$src" ]; then
            process_dir "$src"
        elif [ "$src" != "$dst" ]; then
            process_file "$src" "$dst"
        fi
    done
}

_var_re='{{[a-zA-Z0-9_-]\+}}'
process_file () {
    local reason
    if reason="$(sed -e "$sed_program" <"$1" 2>&1 1>"$2")"; then
        printf '› %s -> %s\n' "$1" "$2"
        if grep -sq "$_var_re" "$2"; then
            warning "Undefined variables in ‘$1’"
            grep>&2 -n "$_var_re" "$2"
        fi
    else
        skip_warning "$1" "$reason"
    fi
}

show_warn_count () {
    test $warn_count -gt 0 || return 0

    exec >&2

    printf '%s: %s %s\n' \
        "$ME" $warn_count $(pluralize $warn_count warning)

    test $warn_count -le 99 || warn_count=99
    exit $warn_count
}

process_arguments () {
    sed_program=''

    for arg; do
        case $arg in
            (--)
                echo shift
                break
                ;;

            (-h|--help)
                echo usage
                return
                ;;

            (-*)
                usage_error 1 "Unrecognized option: ‘$1’"
                ;;

            (=*)
                usage_error 2 "Replacement name cannot be empty: ‘$1’"
                ;;

            (*=*)
                name="$(regexp_quote "${arg%%=*}" @)"
                value="$(regexp_quote "${arg#*=}" @)"
                sed_program="${sed_program}s@{{$name}}@$value@g;"
                echo shift
                ;;

            (*)
                break
                ;;
        esac
    done

    if [ -z "$sed_program" ]; then
        usage_error 3 "Need at least one replacement"
    fi

    echo "sed_program=$(shell_quote "$sed_program")"

    if [ $# = 0 ]; then
        echo 'set -- .'
    fi
}

usage_error () {
    printf '%s: %s\n' "$ME" "$2"
    usage $(($1 + 200))
} >&2

warning () {
    : $(( ++warn_count ))

    printf '%s: Warning: %s%s\n' "$ME" "$1" "${2:+:}"

    if [ -n "${2-}" ]; then
        printf '  Reason: %s\n' "$2"
    fi
} >&2

skip_warning () {
    local file; file="$1"; shift
    set -- "Skipping input file: ‘${file}’" "$@"
    warning "$@"
}

pluralize () {
    if [ "$1" = 1 ]; then
        printf %s "$2"
    else
        printf %s "${3-${2}s}"
    fi
}

dir_is_nonempty () {
    test -z "$(find "$1" -maxdepth 0 -empty)"
}

shell_quote () {
    printf "'%s'" "$(printf '%s\n' "$1" | sed 's/'\''/&\\&&/g')"
}

regexp_quote () {
    printf %s "$1" | sed 's/[].*\\'"${2-/}"'[]/\\&/g'
}

#########
main "$@"
#########
