#!/bin/sh
#
# $1 version to package

set -e

VERSION=$1
SVERSION=`echo $1 | sed 's/^v//'`
git archive --prefix=dist/awesome-$SVERSION/ $VERSION | tar -xf -
cd dist
echo -n $VERSION > awesome-$SVERSION/.version_stamp
tar cjf awesome-$SVERSION.tar.bz2 awesome-$SVERSION
tar cJf awesome-$SVERSION.tar.xz awesome-$SVERSION

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
