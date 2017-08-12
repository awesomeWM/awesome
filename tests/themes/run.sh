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
tests_dir="$(dirname -- "$0")/.."

config_file="$build_dir/test-themes-awesomerc.lua"

for theme in themes/*/theme.lua; do
  echo "== Testing $theme =="
  theme=${theme%/*}
  theme=${theme##*/}

  sed "s~default/theme~$theme/theme~g" "awesomerc.lua" > "$config_file"

  # Set CMAKE_BINARY_DIR for out-of-tree builds.
  CMAKE_BINARY_DIR="$PWD" \
  AWESOME_RC_FILE="$config_file" \
    AWESOME_THEMES_PATH="$PWD/themes" \
    AWESOME_ICON_PATH="$PWD/icons" \
    "$tests_dir/run.sh" themes/tests.lua
done
