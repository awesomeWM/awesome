#!/bin/sh
#
# $1 version to package

set -e

VERSION=$1
SVERSION=`echo $1 | sed 's/^v//'`
git archive --prefix=dist/awesome-$SVERSION/ $VERSION | tar -xf -
cd dist
echo -n $VERSION > awesome-$SVERSION/.version_stamp
tar czf awesome-$SVERSION.tar.gz awesome-$SVERSION
tar cjf awesome-$SVERSION.tar.bz2 awesome-$SVERSION
