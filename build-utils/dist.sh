#!/bin/sh
#
# $1 version to package

set -e

VERSION=`echo $1 | sed 's/^v//'`
git-archive --prefix=dist/awesome-$VERSION/ $VERSION | tar -xf -
cd dist
echo -n $VERSION > awesome-$VERSION/.version_stamp
tar czf awesome-$VERSION.tar.gz awesome-$VERSION
tar cjf awesome-$VERSION.tar.bz2 awesome-$VERSION
