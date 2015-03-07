#!/bin/sh
#
# $1 file containing the last git-describe output
# $2 the file in which to update the version string
#
# TODO: proper error handling

die()
{
        echo "$0: WARNING: version stamp update failed."
        #exit 1 # not important enough to stop the build.
        exit 0
}

STAMP=`cat "$1" 2> /dev/null`
CURRENT=`git describe --dirty 2>/dev/null`

if [ -z "$STAMP" -o -z "$CURRENT" ]
then
     die
fi

if [ "$STAMP" != "$CURRENT" ]
then
    echo "git version changed: $STAMP -> $CURRENT"
    sed -e s/$STAMP/$CURRENT/g "$2" 1> "$2.new" || die
    mv "$2.new" "$2"
    echo -n "$CURRENT" > "$1"
fi
