#!/bin/sh

# Report coverage for each .gcno file separately.
# gcov will create .gcov files for the same source (e.g. for
# globalconf.h from awesome.c.gcno and event.c.gcno).

set -x

source_dir="${SOURCE_DIRECTORY:-$PWD}"
cd "${BUILD_DIRECTORY:-build}" || exit

base="$(mktemp -d --tmpdir=. gcov.XXXX)"

i=0
find . -path "*/lgi-check.dir" -prune -o \( -name '*.gcno' -print \) | while read -r gcno; do
  gcov --source-prefix "$source_dir" -pb "$gcno"

  i=$((i+1))
  dir="$base/$i"
  mkdir "$dir"
  mv ./*.gcov "$dir"

  # Delete any files for /usr.
  # They are not relevant and might cause "Invalid path part" errors
  # with Code Climate.
  find "$dir" -maxdepth 1 -type f -name '#usr#*.gcov' -delete
done
