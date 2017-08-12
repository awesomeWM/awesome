#!/usr/bin/env bash
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

export SHELL=/bin/sh
export HOME=/dev/null

VERBOSE=${VERBOSE-0}
if [ "$VERBOSE" = 1 ]; then
    set -x
fi

# Change to file's dir (POSIXly).
cd -P -- "$(dirname -- "$0")"
this_dir="$PWD"
source_dir="${this_dir%/*}"

# Either the build dir is passed in $CMAKE_BINARY_DIR or we guess based on $PWD
build_dir="$CMAKE_BINARY_DIR"
if [ -z "$build_dir" ]; then
    if [ -d "$source_dir/build" ]; then
        build_dir="$source_dir/build"
    else
        build_dir="$source_dir"
    fi
fi
export build_dir

# Get test files: test*, or the ones provided as args (relative to tests/).
if [ $# != 0 ]; then
    tests="$*"
else
    tests="$this_dir/test*.lua"
fi

# Travis.
if [ "$CI" = true ]; then
    HEADLESS=1
    TEST_PAUSE_ON_ERRORS=0
else
    HEADLESS=${HEADLESS-0}
    TEST_PAUSE_ON_ERRORS=${TEST_PAUSE_ON_ERRORS-0}
fi
export TEST_PAUSE_ON_ERRORS  # Used in tests/_runner.lua.

XEPHYR=Xephyr
XVFB=Xvfb
AWESOME=$build_dir/awesome
if ! [ -x "$AWESOME" ]; then
    echo "$AWESOME is not executable." >&2
    exit 1
fi
AWESOME_CLIENT="$source_dir/utils/awesome-client"
D=:5
SIZE="${TESTS_SCREEN_SIZE:-1024x768}"

# Set up some env vars
# Disable GDK's screen scaling support
export GDK_SCALE=1
# No idea what this does, but it silences a warning that GTK init might print
export NO_AT_BRIDGE=1

if [ $HEADLESS = 1 ]; then
    "$XVFB" $D -noreset -screen 0 "${SIZE}x24" &
    xserver_pid=$!
else
    # export XEPHYR_PAUSE=1000
    "$XEPHYR" $D -ac -name xephyr_$D -noreset -screen "$SIZE" &
    xserver_pid=$!
    # Toggles debugging mode, using XEPHYR_PAUSE.
    # ( sleep 1; kill -USR1 $xserver_pid ) &
fi

# Add test dir (for _runner.lua).
# shellcheck disable=SC2206
awesome_options=($AWESOME_OPTIONS --search lib --search "$this_dir")
export XDG_CONFIG_HOME="$build_dir"

# Cleanup on errors / aborting.
cleanup() {
    for p in $awesome_pid $xserver_pid; do
        kill -TERM "$p" 2>/dev/null || true
    done
    rm -rf "$tmp_files" || true
}
trap "cleanup" 0 2 3 15

tmp_files=$(mktemp -d)
awesome_log=$tmp_files/_awesome_test.log
echo "awesome_log: $awesome_log"

wait_until_success() {
    if [ "$VERBOSE" = 1 ]; then
        set +x
    fi
    wait_count=60  # 60*0.05s => 3s.
    while true; do
        set +e
        eval reply="\$($2)"
        ret=$?
        set -e
        if [ $ret = 0 ]; then
            break
        fi
        wait_count=$((wait_count - 1))
        if [ "$wait_count" -lt 0 ]; then
            echo "Error: failed to $1!" >&2
            # shellcheck disable=SC2154
            echo "Last reply: $reply." >&2
            if [ -f "$awesome_log" ]; then
                echo "Log:" >&2
                cat "$awesome_log" >&2
            fi
            exit 1
        fi
        sleep 0.05
    done
    if [ "$VERBOSE" = 1 ]; then
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
eval "$(DISPLAY="$D" dbus-launch --sh-syntax --exit-with-session)"

RC_FILE=${AWESOME_RC_FILE:-${source_dir}/awesomerc.lua}
AWESOME_THEMES_PATH="${AWESOME_THEMES_PATH:-${source_dir}/themes}"
AWESOME_ICON_PATH="${AWESOME_ICON_PATH:-${source_dir}/icons}"

# Inject coverage runner via temporary RC file.
if [ -n "$DO_COVERAGE" ] && [ "$DO_COVERAGE" != 0 ]; then
    # Handle old filename of config files (useful for git-bisect).
    if [ -f "${RC_FILE}.in" ]; then
        RC_FILE="${RC_FILE}.in"
    fi
    sed "1 s~^~require('luacov.runner')('$source_dir/.luacov'); \0~" \
        "$RC_FILE" > "$tmp_files/awesomerc.lua"
    RC_FILE=$tmp_files/awesomerc.lua
fi

# Start awesome.
start_awesome() {
    cd "$build_dir"
    # Kill awesome after $timeout_stale seconds (e.g. for errors during test setup).
    # SOURCE_DIRECTORY is used by .luacov.
    DISPLAY="$D" SOURCE_DIRECTORY="$source_dir" \
        AWESOME_THEMES_PATH="$AWESOME_THEMES_PATH" \
        AWESOME_ICON_PATH="$AWESOME_ICON_PATH" \
        timeout "$timeout_stale" "$AWESOME" -c "$RC_FILE" "${awesome_options[@]}" > "$awesome_log" 2>&1 &
    awesome_pid=$!
    cd - >/dev/null

    # Wait until the interface for awesome-client is ready (D-Bus interface).
    # Do this with dbus-send so that we can specify a low --reply-timeout
    wait_until_success "wait for awesome startup via awesome-client" "dbus-send --reply-timeout=100 --dest=org.awesomewm.awful --print-reply / org.awesomewm.awful.Remote.Eval 'string:return 1' 2>&1"
}

if command -v tput >/dev/null; then
    color_red() { tput setaf 1; }
    color_reset() { tput sgr0; }
else
    color_red() { :; }
    color_reset() { :; }
fi

count_tests=0
errors=()
# Seconds after when awesome gets killed.
timeout_stale=180 # FIXME This should be no more than 60s

for f in $tests; do
    echo "== Running $f =="
    (( ++count_tests ))

    start_awesome

    if [ ! -r "$f" ]; then
        if [ -r "${f#tests/}" ]; then
            f=${f#tests/}
        else
            echo "===> ERROR $f is not readable! <==="
            errors+=("$f is not readable.")
            continue
        fi
    fi

    # Send the test file to awesome.
    DISPLAY=$D "$AWESOME_CLIENT" 2>&1 < "$f"

    # Tail the log and quit, when awesome quits.
    # Use a single `grep`, otherwise `--line-buffered` would be required.
    tail -n 100000 -s 0.1 -f --pid "$awesome_pid" "$awesome_log" \
        | grep -vE '^(.{19} W: awesome: a_dbus_connect:[0-9]+: Could not connect to D-Bus system bus:|Test finished successfully\.$)' || true

    set +e
    wait $awesome_pid
    code=$?
    set -e
    case $code in
        0) ;;
        124) echo "Awesome was killed due to timeout after $timeout_stale seconds" ;;
        *) echo "Awesome exited with status code $code" ;;
    esac

    # Parse any error from the log.
    error="$(grep --color -o --binary-files=text -E \
        '.*[Ee]rror.*|.*assertion failed.*|^Step .* failed:' "$awesome_log" ||
        true)"
    if [[ -n "$error" ]]; then
        color_red
        echo "===> ERROR running $f <==="
        echo "$error"
        color_reset
        errors+=("$f: $error")
    elif ! grep -q -E '^Test finished successfully\.$' "$awesome_log"; then
        color_red
        echo "===> ERROR running $f <==="
        color_reset
        errors+=("$f: test did not indicate success. See the output above.")
    fi
done

echo "$count_tests tests finished."

if (( "${#errors[@]}" )); then
    if [ "$TEST_PAUSE_ON_ERRORS" = 1 ]; then
        echo "Pausing... press Enter to continue."
        read -r
    fi
    color_red
    echo "There were ${#errors[@]} errors:"
    for error in "${errors[@]}"; do
        echo " - $error"
    done
    color_reset
    exit 1
fi
exit 0

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
