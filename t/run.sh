#!/bin/sh

set -e

# Change to current dir (POSIXly).
cd -P -- "$(dirname -- "$0")"
this_dir=$PWD

# Use a separate D-Bus session.
eval $(dbus-launch --sh-syntax)

root_dir=$PWD/..

XEPHYR=Xephyr
AWESOME=$root_dir/build/awesome
RC_FILE=$root_dir/build/awesomerc.lua
D=:5
SIZE=1024x768

# export XEPHYR_PAUSE=1000
"$XEPHYR" $D -ac -name xephyr_$D -noreset -screen "$SIZE" $XEPHYR_OPTIONS &
sleep 1
xephyr_pid=$(pgrep -n Xephyr)
# Toggles debugging mode, using XEPHYR_PAUSE.
# pkill -USR1 Xephyr


cd $root_dir/build

LUA_PATH="$(lua -e 'print(package.path)');lib/?.lua;lib/?/init.lua"
XDG_CONFIG_HOME="./"
export LUA_PATH
export XDG_CONFIG_HOME

# awesome_log=$(mktemp)
awesome_log=/tmp/_awesome_test.log
echo "awesome_log: $awesome_log"

# Start awesome.
DISPLAY=$D "$AWESOME" -c "$RC_FILE" $AWESOME_OPTIONS > $awesome_log 2>&1 &
sleep 1
awesome_pid=$(pgrep -n awesome)

cd -

# Cleanup on exit.
trap "set +e; kill -TERM $awesome_pid 2>/dev/null ; kill -TERM $xephyr_pid 2>/dev/null" 0 2 3 15

restart_awesome=0
errors=0
for f in $this_dir/*.lua; do
    if [ $restart_awesome = 1 ]; then
        kill -HUP $awesome_pid
    fi

    # Send the test file to awesome.
    cat $f | DISPLAY=$D awesome-client 2>&1 || true

    # Tail the log and quit, when awesome quits.
    tail -f --pid $awesome_pid $awesome_log

    if grep -q -E '^Error|assertion failed' $awesome_log; then
        echo "===> ERROR running $f! <==="
        grep --color -o --binary-files=text -E '^Error.*|.*assertion failed.*' $awesome_log
        errors=$(expr $errors + 1)
    fi

    reload=1
done

kill $xephyr_pid

[ $errors = 0 ] && exit 0 || exit 1
