#!/bin/sh
#
# $1 version to package

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 VERSION"
    exit 64
fi

VERSION=$1
SVERSION=$(echo "$VERSION" | sed 's/^v//')
git archive --prefix "dist/awesome-$SVERSION/" "$VERSION" | tar -xf -
cd dist
printf '%s' "$VERSION" > "awesome-$SVERSION/.version_stamp"
tar cjf "awesome-$SVERSION.tar.bz2" "awesome-$SVERSION"
tar cJf "awesome-$SVERSION.tar.xz" "awesome-$SVERSION"
gpg --armor --detach-sign "awesome-$SVERSION.tar.bz2"
gpg --armor --detach-sign "awesome-$SVERSION.tar.xz"

echo "Created the following files in dist/:"
ls -l "awesome-$SVERSION.tar.bz2"     "awesome-$SVERSION.tar.xz" \
      "awesome-$SVERSION.tar.bz2.asc" "awesome-$SVERSION.tar.xz.asc"

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
