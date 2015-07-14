#!/bin/sh

set -e

# Be verbose on Travis.
if [ "$CI" = true ]; then
    set -x
fi

# Change to file's dir (POSIXly).
cd -P -- "$(dirname -- "$0")"
this_dir=$PWD

# Get test files: test*, or the ones provided as args (relative to tests/).
if [ $# != 0 ]; then
    tests="$@"
else
    tests=$this_dir/test*.lua
fi

# Use a separate D-Bus session; sets $DBUS_SESSION_BUS_PID.
eval $(dbus-launch --sh-syntax)

root_dir=$PWD/..

# Travis.
if [ "$CI" = true ]; then
    HEADLESS=1
    TEST_PAUSE_ON_ERRORS=0
    TEST_QUIT_ON_TIMEOUT=1
else
    HEADLESS=0
    TEST_PAUSE_ON_ERRORS=0
    TEST_QUIT_ON_TIMEOUT=1
fi
export TEST_PAUSE_ON_ERRORS

XEPHYR=Xephyr
XVFB=Xvfb
AWESOME=$root_dir/build/awesome
RC_FILE=$root_dir/build/awesomerc.lua
D=:5
SIZE=1024x768

if [ $HEADLESS = 1 ]; then
    "$XVFB" $D -screen 0 ${SIZE}x24 &
    sleep 1
    xserver_pid=$(pgrep -n Xvfb)
else
    # export XEPHYR_PAUSE=1000
    # if [ -f /tmp/.X5-lock ]; then
    #     echo "Xephyr is already running for display $D.. aborting." >&2
    #     exit 1
    # fi
    "$XEPHYR" $D -ac -name xephyr_$D -noreset -screen "$SIZE" $XEPHYR_OPTIONS &
    sleep 1
    xserver_pid=$(pgrep -n Xephyr)
fi
# Toggles debugging mode, using XEPHYR_PAUSE.
# pkill -USR1 Xephyr


cd $root_dir/build

LUA_PATH="$(lua -e 'print(package.path)');lib/?.lua;lib/?/init.lua"
# Add test dir (for _runner.lua).
LUA_PATH="$LUA_PATH;$this_dir/?.lua"
XDG_CONFIG_HOME="./"
export LUA_PATH
export XDG_CONFIG_HOME

# awesome_log=$(mktemp)
awesome_log=/tmp/_awesome_test.log
echo "awesome_log: $awesome_log"

cd - >/dev/null


kill_childs() {
    for p in $awesome_pid $xserver_pid $DBUS_SESSION_BUS_PID; do
        kill -TERM $p 2>/dev/null || true
    done
}
# Cleanup on errors / aborting.
set_trap() {
    trap "kill_childs" 0 2 3 15
}
set_trap

AWESOME_CLIENT="$root_dir/utils/awesome-client"

# Start awesome.
start_awesome() {
    (
        export DISPLAY="$D"
        cd $root_dir/build
        # Setup xrdb, for awesome's xresources backend / queries.
        echo "Xft.dpi: 96" | xrdb
        "$AWESOME" -c "$RC_FILE" $AWESOME_OPTIONS > $awesome_log 2>&1 &
    )

    # Get PID of awesome.
    awesome_pid=
    max_wait=30
    while true; do
        awesome_pid="$(pgrep -nf "awesome -c $RC_FILE")"
        if [ -n "$awesome_pid" ]; then
            break;
        fi
        max_wait=$(expr $max_wait - 1)
        if [ "$max_wait" -lt 0 ]; then
            echo "Error: Failed to start awesome (-c $RC_FILE)!"
            echo "Log:"
            cat "$awesome_log"
            exit 1
        fi
        sleep 0.1
    done

    # Wait until the interface for awesome-client is ready (D-Bus interface).
    client_reply=
    max_wait=50
    while true; do
        set +e
        client_reply=$(echo 'return 1' | DISPLAY=$D "$AWESOME_CLIENT" 2>&1)
        ret=$?
        set -e
        if [ $ret = 0 ]; then
            break
        fi
        max_wait=$(expr $max_wait - 1)
        if [ "$max_wait" -lt 0 ]; then
            echo "Error: did not receive a successful reply via awesome-client!"
            echo "Last reply: $client_reply."
            echo "Log:"
            cat "$awesome_log"
            exit 1
        fi
        sleep 0.1
    done
}

# Count errors.
errors=0

for f in $tests; do
    echo "== Running $f =="
    start_awesome

    # Send the test file to awesome.
    cat $f | DISPLAY=$D "$AWESOME_CLIENT" 2>&1

    # Tail the log and quit, when awesome quits.
    tail -f --pid $awesome_pid $awesome_log

    if grep -q -E '^Error|assertion failed' $awesome_log; then
        echo "===> ERROR running $f! <==="
        grep --color -o --binary-files=text -E '^Error.*|.*assertion failed.*' $awesome_log
        errors=$(expr $errors + 1)

        if [ "$TEST_PAUSE_ON_ERRORS" = 1 ]; then
            echo "Pausing... press Enter to continue."
            read enter
        fi
    fi
done

[ $errors = 0 ] && exit 0 || exit 1
