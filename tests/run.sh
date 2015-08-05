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
AWESOME_CLIENT="$root_dir/utils/awesome-client"
D=:5
SIZE=1024x768

if [ $HEADLESS = 1 ]; then
    "$XVFB" $D -screen 0 ${SIZE}x24 &
    xserver_pid=$!
else
    # export XEPHYR_PAUSE=1000
    "$XEPHYR" $D -ac -name xephyr_$D -noreset -screen "$SIZE" $XEPHYR_OPTIONS &
    xserver_pid=$!
    # Toggles debugging mode, using XEPHYR_PAUSE.
    # ( sleep 1; kill -USR1 $xserver_pid ) &
fi

cd $root_dir/build

LUA_PATH="$(lua -e 'print(package.path)');lib/?.lua;lib/?/init.lua"
# Add test dir (for _runner.lua).
LUA_PATH="$LUA_PATH;$this_dir/?.lua"
XDG_CONFIG_HOME="./"
export LUA_PATH
export XDG_CONFIG_HOME

cd - >/dev/null

# Cleanup on errors / aborting.
cleanup() {
    for p in $awesome_pid $xserver_pid; do
        kill -TERM $p 2>/dev/null || true
    done
    rm -rf $tmp_files || true
}
trap "cleanup" 0 2 3 15

tmp_files=$(mktemp -d)
awesome_log=$tmp_files/_awesome_test.log
echo "awesome_log: $awesome_log"

# Wait for DISPLAY to be available, and setup xrdb,
# for awesome's xresources backend / queries.
max_wait=60
while true; do
    set +e
    reply="$(echo "Xft.dpi: 96" | DISPLAY="$D" xrdb 2>&1)"
    ret=$?
    set -e
    if [ $ret = 0 ]; then
        break
    fi
    max_wait=$(expr $max_wait - 1)
    if [ "$max_wait" -lt 0 ]; then
        echo "Error: failed to setup xrdb!"
        echo "Last reply: $reply."
        echo "Log:"
        cat "$awesome_log"
        exit 1
    fi
    sleep 0.05
done

# Use a separate D-Bus session; sets $DBUS_SESSION_BUS_PID.
eval $(DISPLAY="$D" dbus-launch --sh-syntax --exit-with-session)

# Not in Travis?
if [ "$CI" != true ]; then
    # Prepare a config file pointing to a working theme
    RC_FILE=$tmp_files/awesomerc.lua
    THEME_FILE=$tmp_files/theme.lua
    sed -e "s:beautiful.init(\"@AWESOME_THEMES_PATH@/default/theme.lua\"):beautiful.init('$THEME_FILE'):" $root_dir/awesomerc.lua > $RC_FILE
    sed -e "s:@AWESOME_THEMES_PATH@/default/titlebar:$root_dir/build/themes/default/titlebar:"  \
        -e "s:@AWESOME_THEMES_PATH@:$root_dir/themes/:" \
        -e "s:@AWESOME_ICON_PATH@:$root_dir/icons:" $root_dir/themes/default/theme.lua > $THEME_FILE
fi

# Start awesome.
start_awesome() {
    export DISPLAY="$D"
    cd $root_dir/build
    DISPLAY="$D" "$AWESOME" -c "$RC_FILE" $AWESOME_OPTIONS > $awesome_log 2>&1 &
    awesome_pid=$!
    cd - >/dev/null

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
        max_wait=$(expr $max_wait - 1 || true)
        if [ "$max_wait" -lt 0 ] || ! kill -0 $awesome_pid ; then
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

    if [ ! -r $f ]; then
        echo "===> ERROR $f is not readable! <==="
        errors=$(expr $errors + 1)
        continue
    fi

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
