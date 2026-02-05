#!/bin/bash
# Wrapper script for running a command and failing if output contains "ERROR:".

set -euo pipefail

# Create a temporary file for the output
output_file=$(mktemp)
trap 'rm -f "$output_file"' EXIT

"$@" 2>&1 | tee "$output_file"
cmd_exit_code=${PIPESTATUS[0]}

if grep -qE "^(ERROR|WARNING):" "$output_file"; then
    echo "ERROR: ldoc reported errors or warnings"
    exit 1
fi

exit "$cmd_exit_code"
