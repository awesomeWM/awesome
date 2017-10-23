#!/bin/sh
#
# Test all themes.
# This should be run via `make check-themes` (or manually from the build
# directory).

set -e

if ! [ -f CMakeLists.txt ]; then
    echo 'This should be run from the source directory (expected CMakeLists.txt).' >&2
    exit 64
fi
source_dir="$PWD"

# Either the build dir is passed in $CMAKE_BINARY_DIR or we guess based on $PWD
# Same as in tests/run.sh.
build_dir="$CMAKE_BINARY_DIR"
if [ -z "$build_dir" ]; then
    if [ -d "$source_dir/build" ]; then
        build_dir="$source_dir/build"
    else
        build_dir="$source_dir"
    fi
fi

config_file="$(mktemp)"

# Cleanup on errors / aborting.
cleanup() {
    rm "$config_file" || true
}
trap "cleanup" 0 2 3 15

for theme_file in themes/*/theme.lua; do
    echo "==== Testing theme: $theme_file ===="
    theme_name=${theme_file%/*}
    theme_name=${theme_name##*/}

    sed "s~default/theme~$theme_name/theme~g" "awesomerc.lua" > "$config_file"

    AWESOME_RC_FILE="$config_file" \
        AWESOME_THEMES_PATH="$build_dir/themes" \
        AWESOME_ICON_PATH="$PWD/icons" \
        "$source_dir/tests/run.sh" "$source_dir/tests/themes/tests.lua"
done
