#!/bin/sh

set -eu

project_root=$(dirname "$(dirname "$0")")
version_file=$project_root/.version
replace_prog=$project_root/scripts/replace.sh

if [ $# = 0 ]; then
    set -- "$project_root"
fi

. "$version_file"

version=$major_version.$minor_version.$patch_version
date=$(date +'%B %-d, %Y')

"$replace_prog" \
    major_version="$major_version" \
    minor_version="$minor_version" \
    patch_version="$patch_version" \
    version="$version" \
    date="$date" \
    "$@"
