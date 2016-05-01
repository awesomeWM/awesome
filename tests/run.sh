#!/bin/sh
#
# Test runner.
#
# This can also be used to start awesome from the build directory, e.g. for
# git-bisect:
# 1. Put the file into a subdirectory, which is ignored by git, e.g.
#    `tmp/run.sh`.
# 2. Run it from the source root (after `make`):
#    env TEST_PAUSE_ON_ERRORS=1 sh tmp/run.sh
# It should start Xephyr and launch awesome in it, using the default config.

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
    TEST_PAUSE_ON_ERRORS=${TEST_PAUSE_ON_ERRORS-0}
    TEST_QUIT_ON_TIMEOUT=1
fi
export TEST_PAUSE_ON_ERRORS  # Used in tests/_runner.lua.

XEPHYR=Xephyr
XVFB=Xvfb
AWESOME=$root_dir/build/awesome
RC_FILE=$root_dir/build/awesomerc.lua
AWESOME_CLIENT="$root_dir/utils/awesome-client"
D=:5
SIZE="${TESTS_SCREEN_SIZE:-1024x768}"

if [ $HEADLESS = 1 ]; then
    "$XVFB" $D -noreset -screen 0 ${SIZE}x24 &
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

wait_until_success() {
    if [ "$CI" = true ]; then
        set +x
    fi
    max_wait=60
    while true; do
        set +e
        eval reply="\$($2)"
        ret=$?
        set -e
        if [ $ret = 0 ]; then
            break
        fi
        max_wait=$(expr $max_wait - 1 || true)
        if [ "$max_wait" -lt 0 ]; then
            echo "Error: failed to $1!"
            echo "Last reply: $reply."
            if [ -f "$awesome_log" ]; then
                echo "Log:"
                cat "$awesome_log"
            fi
            exit 1
        fi
        sleep 0.05
    done
    if [ "$CI" = true ]; then
        set -x
    fi
}

# Wait for DISPLAY to be available, and setup xrdb,
# for awesome's xresources backend / queries.
wait_until_success "setup xrdb" "printf 'Xft.dpi: 96
    *.background: #002b36
    *.foreground: #93a1a1
    *.color0:     #002b36
    *.color1:     #dc322f
    *.color10:    #859900
    *.color11:    #b58900
    *.color12:    #268bd2
    *.color13:    #6c71c4
    *.color14:    #2aa198
    *.color15:    #fdf6e3
    *.color16:    #cb4b16
    *.color17:    #d33682
    *.color18:    #073642
    *.color19:    #586e75
    *.color2:     #859900
    *.color20:    #839496
    *.color21:    #eee8d5
    *.color3:     #b58900
    *.color4:     #268bd2
    *.color5:     #6c71c4
    *.color6:     #2aa198
    *.color7:     #93a1a1
    *.color8:     #657b83
    *.color9:     #dc322f' | DISPLAY='$D' xrdb 2>&1"

# Use a separate D-Bus session; sets $DBUS_SESSION_BUS_PID.
eval $(DISPLAY="$D" dbus-launch --sh-syntax --exit-with-session)

# Not in Travis?
if [ "$CI" != true ]; then
    # Prepare a config file pointing to a working theme
    # Handle old filename of config files (useful for git-bisect).
    if [ -f $root_dir/awesomerc.lua.in ]; then
        SED_IN=.in
    fi
    RC_FILE=$tmp_files/awesomerc.lua
    THEME_FILE=$tmp_files/theme.lua
    sed -e "s:.*beautiful.init(.*default/theme.lua.*:beautiful.init('$THEME_FILE'):" $root_dir/awesomerc.lua$SED_IN > $RC_FILE
    sed -e "s:@AWESOME_THEMES_PATH@/default/titlebar:$root_dir/build/themes/default/titlebar:"  \
        -e "s:@AWESOME_THEMES_PATH@:$root_dir/themes/:" \
        -e "s:@AWESOME_ICON_PATH@:$root_dir/icons:" $root_dir/themes/default/theme.lua$SED_IN > $THEME_FILE
fi

# Start awesome.
start_awesome() {
    export DISPLAY="$D"
    cd $root_dir/build
    DISPLAY="$D" "$AWESOME" -c "$RC_FILE" $AWESOME_OPTIONS > $awesome_log 2>&1 &
    awesome_pid=$!
    cd - >/dev/null

    # Wait until the interface for awesome-client is ready (D-Bus interface).
    wait_until_success "wait for awesome startup via awesome-client" "echo 'return 1' | DISPLAY=$D '$AWESOME_CLIENT' 2>&1"
}

# Count errors.
errors=0

for f in $tests; do
    echo "== Running $f =="

    start_awesome

    if [ ! -r $f ]; then
        echo "===> ERROR $f is not readable! <==="
        errors=$(expr $errors + 1)
        continue
    fi

    # Send the test file to awesome.
    cat $f | DISPLAY=$D "$AWESOME_CLIENT" 2>&1

    # Tail the log and quit, when awesome quits.
    tail -n 100000 -f --pid $awesome_pid $awesome_log

    if ! grep -q -E '^Test finished successfully$' $awesome_log ||
            grep -q -E '[Ee]rror|assertion failed' $awesome_log; then
        echo "===> ERROR running $f! <==="
        grep --color -o --binary-files=text -E '.*[Ee]rror.*|.*assertion failed.*' $awesome_log
        errors=$(expr $errors + 1)
    fi
done

if ! [ $errors = 0 ]; then
    if [ "$TEST_PAUSE_ON_ERRORS" = 1 ]; then
        echo "Pausing... press Enter to continue."
        read enter
    fi
    echo "There were $errors errors!"
    exit 1
fi
exit 0

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
