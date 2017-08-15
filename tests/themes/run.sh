#!/bin/sh
#
# Test all themes.
# This should be run via `make check-themes` (or manually from the build
# directory).

set -e

if ! [ -f CMakeCache.txt ]; then
  echo 'This should be run from the build directory (expected CMakeCache.txt).' >&2
  exit 64
fi
build_dir="$PWD"
src_dir="$(dirname -- "$0")/../.."

config_file="$build_dir/test-themes-awesomerc.lua"

for theme_file in themes/*/theme.lua; do
  echo "==== Testing theme: $theme_file ===="
  theme_name=${theme_file%/*}
  theme_name=${theme_name##*/}

  sed "s~default/theme~$theme_name/theme~g" "awesomerc.lua" > "$config_file"

  # Set CMAKE_BINARY_DIR for out-of-tree builds.
  CMAKE_BINARY_DIR="$PWD" \
  AWESOME_RC_FILE="$config_file" \
    AWESOME_THEMES_PATH="$src_dir/themes" \
    AWESOME_ICON_PATH="$PWD/icons" \
    "$src_dir/tests/run.sh" themes/tests.lua
done
