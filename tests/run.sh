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
export HOME="${TMP:-/tmp}"

# Parse options.
usage() {
    cat >&2 <<EOF
Usage: $0 [OPTION]... [FILE]...

Options:
  -v: verbose mode
  -W: warnings become errors
  -m: Use --screen off
  -h: show this help
EOF
    exit "$1"
}
fail_on_warning=
manual_screens=
verbose=${VERBOSE:-0}
while getopts vWmh opt; do
    case $opt in
        v) verbose=1 ;;
        W) fail_on_warning=1 ;;
        m) manual_screens=" --screen off" ;;
        h) usage 0 ;;
        *) usage 64 ;;
    esac
done
shift $((OPTIND-1))

if (( verbose )); then
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
if ! $AWESOME --version; then
    echo "$AWESOME cannot be run." >&2
    exit 1
fi
AWESOME_CLIENT="$source_dir/utils/awesome-client"
D=:5
SIZE="${TESTS_SCREEN_SIZE:-1024x768}"
# Seconds after when awesome gets killed.
TEST_TIMEOUT=${TEST_TIMEOUT:-30}

# Set up some env vars
# Disable GDK's screen scaling support
export GDK_SCALE=1
# No idea what this does, but it silences a warning that GTK init might print
export NO_AT_BRIDGE=1

# Enable partial maximization tests if wmctrl is found.
if command -v wmctrl >/dev/null 2>&1; then
    export HAS_WMCTRL=1
fi

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
awesome_options=($AWESOME_OPTIONS $manual_screens --search lib --search "$this_dir")

awesome_options+=(--screen off --force)

# Cleanup on errors / aborting.
cleanup() {
    for p in $awesome_pid $xserver_pid; do
        kill -TERM "$p" 2>/dev/null || true
        wait "$p"
    done
    rm -rf "$tmp_files" || true
}
trap "cleanup" 0 2 3 15

tmp_files=$(mktemp -d)
awesome_log=$tmp_files/_awesome_test.log
echo "awesome_log: $awesome_log"

wait_until_success() {
    if (( verbose )); then set +x; fi
    wait_count=$(echo "${TEST_TIMEOUT} / 0.05" | bc)
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
    if (( verbose )); then set -x; fi
}

# Wait for DISPLAY to be available, i.e. the X server is running.
wait_until_success "X resources are ready" "DISPLAY='$D' xrdb -q >/dev/null 2>&1"

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

    # On some systems clients from a test may still linger for a while until they
    # are fully killed. Since this can affect susequent tests, we wait until all
    # of them are gone.
    # The check command needs to call `xlsclients` twice:
    # 1. Test the stdout of `xlsclients` and make sure there are no clients left.
    # 2. Check and fail if `xlsclients` itself fails. Print its stderr, if any.
    # This is due to the fact that, if `xlsclient` return non-0, the `wc` would
    # still report `0`. The order of commands makes sure that `xlsclient` is only
    # called twice when needed. If there are still clients left, the `[` will
    # shotcurcuit.
    wait_until_success \
        'wait for X clients from previous test to close' \
        "[ \$(DISPLAY="$D" xlsclients | wc -l) -eq 0 ] && DISPLAY="$D" xlsclients > /dev/null"

    # Kill awesome after $TEST_TIMEOUT seconds (e.g. for errors during test setup).
    # SOURCE_DIRECTORY is used by .luacov.
    DISPLAY="$D" SOURCE_DIRECTORY="$source_dir" \
        AWESOME_THEMES_PATH="$AWESOME_THEMES_PATH" \
        AWESOME_ICON_PATH="$AWESOME_ICON_PATH" \
        XDG_CONFIG_HOME="$build_dir" \
        timeout "$TEST_TIMEOUT" "$AWESOME" -c "$RC_FILE" "${awesome_options[@]}" > "$awesome_log" 2>&1 &
    awesome_pid=$!
    cd - >/dev/null

    # Wait until the interface for awesome-client is ready (D-Bus interface).
    # Do this with dbus-send so that we can specify a low --reply-timeout
    wait_until_success "wait for awesome startup via awesome-client" "dbus-send --reply-timeout=${TEST_TIMEOUT} --dest=org.awesomewm.awful --print-reply / org.awesomewm.awful.Remote.Eval 'string:return 1' 2>&1"
}

# Only print colors when stdout is connected to a terminal.
# When it is piped/redirected, skip the color codes.
if [ -t 1 ]; then
    color_red() { echo -e "\e[31m"; }
    color_reset() { echo -e "\e[0m"; }
else
    color_red() { :; }
    color_reset() { :; }
fi

count_tests=0
errors=()

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

    # Make the filename absolute if it is not.
    if [ "${f#/}" = "$f" ]; then
        f="$PWD/$f"
    fi

    # Execute the test file in awesome.
    DISPLAY=$D "$AWESOME_CLIENT" 2>&1 "dofile('$f')" || echo "Error: awesome-client failed" >> "$awesome_log"

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
        124) echo "Awesome was killed due to timeout after $TEST_TIMEOUT seconds" ;;
        *) echo "Awesome exited with status code $code" ;;
    esac

    # Parse any error from the log.
    pattern='.*[Ee]rror.*|.*assertion failed.*|^Step .* failed:|^.{19} E: awesome: .*|.*luaA_panic.*'
    if [[ $fail_on_warning ]]; then
        pattern+='|^.{19} W: awesome:.*'
    fi
    error="$(grep --color -o --binary-files=text -E "$pattern" "$awesome_log" || true)"
    # Filter out false positive errors:
    error="$(echo "$error" | grep -vE ".{19} W: awesome: (Can't read color .* from GTK)" || true)"
    if [[ $fail_on_warning ]]; then
        # Filter out ignored warnings.
        error="$(echo "$error" | grep -vE ".{19} W: awesome: (Removing last screen through fake_remove|a_glib_poll|Cannot reliably detect EOF|beautiful: can't get colorscheme from xrdb|Can't read color .* from GTK+3 theme|A notification|Notification)" || true)"
    fi
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
