#!/bin/sh
# Todo: Give this shell script a sane calling convention. Right now it has a
# file containing the expected output as first argument and just executes
# everything else.

set -ue

# Cleanup on errors / aborting
cleanup() {
    rm -rf "$file_stderr" "$file_stdout" || true
}
trap "cleanup" 0 2 3 15

file_stdout=$(mktemp)
file_stderr=$(mktemp)

expected_output=$1
shift

# Run the command that we were given
exit_code=0
"$@" > ${file_stdout} 2> ${file_stderr} || exit_code=$?

# If exit code is not zero or anything was produced on stderr...
if [ $exit_code -ne 0 -o -s "${file_stderr}" ]
then
    echo "Result: ${exit_code}"
    if [ -s "${file_stdout}" ]
    then
        echo "Output:"
        cat "${file_stdout}"
    fi
    if [ -s "${file_stderr}" ]
    then
        echo "Error:"
        cat "${file_stderr}"
    fi
    exit 1
fi

# Automatically add the output for new examples
if [ ! -e "${expected_output}" ]
then
    cp "${file_stdout}" "${expected_output}"
fi

# Check if we got the output we wanted
if ! cmp -s "${file_stdout}" "${expected_output}"
then
    echo "Expected text from ${expected_output}, but got:"
    diff -u "${expected_output}" "${file_stdout}" || true
    cp "${file_stdout}" "${expected_output}"
    exit 1
fi

exit 0

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
