#!/bin/sh
#
# $1 version to package

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <GIT_TAG>"
    exit 64
fi

GIT_TAG="$1"
SVERSION=$(echo "$GIT_TAG" | sed 's/^v//')

date=$(git log -1 --format=%cI "$GIT_TAG")
git archive --prefix "dist/awesome-$SVERSION/" "$GIT_TAG" | tar -xf -

cd dist
version_stamp="awesome-$SVERSION/.version_stamp"
printf '%s' "$GIT_TAG" > "$version_stamp"
touch --date="$date" "$version_stamp" "awesome-$SVERSION"

tar cf "awesome-$SVERSION.tar" "awesome-$SVERSION"
bzip2 -c "awesome-$SVERSION.tar" > "awesome-$SVERSION.tar.bz2"
xz -c "awesome-$SVERSION.tar" > "awesome-$SVERSION.tar.xz"
rm "awesome-$SVERSION.tar"

gpg --armor --detach-sign "awesome-$SVERSION.tar.bz2"
gpg --armor --detach-sign "awesome-$SVERSION.tar.xz"

echo "Created the following files in dist/:"
ls -l "awesome-$SVERSION.tar.bz2"     "awesome-$SVERSION.tar.xz" \
      "awesome-$SVERSION.tar.bz2.asc" "awesome-$SVERSION.tar.xz.asc"

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
