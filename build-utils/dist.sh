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
gpg --armor --detach-sign awesome-$SVERSION.tar.bz2
gpg --armor --detach-sign awesome-$SVERSION.tar.xz

echo "Created the following files in dist/:"
echo "awesome-$SVERSION.tar.bz2"
echo "awesome-$SVERSION.tar.xz"
echo "awesome-$SVERSION.tar.bz2.asc"
echo "awesome-$SVERSION.tar.xz.asc"

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
