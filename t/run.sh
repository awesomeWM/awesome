#!/bin/sh

set -e
set -x

# Change to current dir (POSIXly).
cd -P -- "$(dirname -- "$0")"
this_dir=$PWD

# Use a separate D-Bus session.
eval $(dbus-launch --sh-syntax)

root_dir=$PWD/..

# Travis.
if [ "$CI" = true ]; then
    HEADLESS=1
else
    HEADLESS=0
fi

XEPHYR=Xephyr
XVFB=Xvfb
AWESOME=$root_dir/build/awesome
RC_FILE=$root_dir/build/awesomerc.lua
D=:5
SIZE=1024x768

if [ $HEADLESS = 1 ]; then
    "$XVFB" $D &
    xserver_pid=$(pgrep -n Xvfb)
else
    # export XEPHYR_PAUSE=1000
    "$XEPHYR" $D -ac -name xephyr_$D -noreset -screen "$SIZE" $XEPHYR_OPTIONS &
    sleep 1
    xserver_pid=$(pgrep -n Xephyr)
fi
# Toggles debugging mode, using XEPHYR_PAUSE.
# pkill -USR1 Xephyr


cd $root_dir/build

LUA_PATH="$(lua -e 'print(package.path)');lib/?.lua;lib/?/init.lua"
# Add test dir (for _runner.lua).
LUA_PATH="$LUA_PATH;../t/?.lua"
XDG_CONFIG_HOME="./"
export LUA_PATH
export XDG_CONFIG_HOME

# awesome_log=$(mktemp)
awesome_log=/tmp/_awesome_test.log
echo "awesome_log: $awesome_log"

cd -


kill_childs() {
    for p in $awesome_pid $xserver_pid; do
        kill -TERM $p 2>/dev/null || true
    done
}
# Cleanup on errors / aborting.
set_trap() {
    trap "kill_childs" 2 3 15
}
set_trap

# Start awesome.
start_awesome() {
    (cd $root_dir/build; \
        DISPLAY=$D "$AWESOME" -c "$RC_FILE" $AWESOME_OPTIONS > $awesome_log 2>&1 || true &)
    sleep 1
    awesome_pid=$(pgrep -nf "awesome -c $RC_FILE" || true)

    if [ -z $awesome_pid ]; then
        echo "Error: Failed to start awesome (-c $RC_FILE)!"
        echo "Log:"
        cat "$awesome_log"
        exit 1
    fi
    set_trap
}

# Count errors.
errors=0

# Get test files: test*, or the ones provided as args.
if [ $# != 0 ]; then
    tests="$@"
else
    tests=$this_dir/test*.lua
fi

for f in $tests; do
    echo "== Running $f =="
    start_awesome

    # Send the test file to awesome.
    cat $f | DISPLAY=$D $root_dir/utils/awesome-client 2>&1

    # Tail the log and quit, when awesome quits.
    tail -f --pid $awesome_pid $awesome_log

    if grep -q -E '^Error|assertion failed' $awesome_log; then
        echo "===> ERROR running $f! <==="
        grep --color -o --binary-files=text -E '^Error.*|.*assertion failed.*' $awesome_log
        errors=$(expr $errors + 1)
    fi
done

kill_childs

[ $errors = 0 ] && exit 0 || exit 1
