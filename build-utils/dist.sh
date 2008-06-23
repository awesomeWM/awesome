#!/bin/sh
#
# $1 version to package

set -e

VERSION=$1
git-archive --prefix=dist/awesome-$VERSION/ $VERSION | tar -xf -
cd dist
echo -n $VERSION > awesome-$VERSION/.version_stamp
tar czf awesome-$VERSION.tar.gz awesome-$VERSION
