#!/usr/bin/env bash

set -o errexit -o pipefail -o xtrace

base_ref="${1:-}"
head_ref="${2:-}"
skip_clean_check="${3:-}"

if [ -z "$base_ref" ] || [ -z "$head_ref" ]; then
  echo "Usage: $0 [<remote>/]<base_ref> [<remote>/]<head_ref> [--skip-clean-check]"
  echo "Optional env var: CMAKE_CONFIG='-DKEY=VALUE ...'"
  echo "Example: $0 origin/master my-feature-branch"
  echo "Example (skip safeguard): $0 origin/master my-feature-branch --skip-clean-check"
  exit 2
fi

if [ -n "$skip_clean_check" ] && [ "$skip_clean_check" != "--skip-clean-check" ]; then
  echo "Unknown third argument: $skip_clean_check"
  echo "Expected: --skip-clean-check"
  exit 2
fi

# Safeguard: Refuse to run on a dirty repository to avoid clobbering local work.
if [ -z "$skip_clean_check" ] && [ -n "$(git status --porcelain --untracked-files=normal)" ]; then
  echo "Repository is not clean. Commit, stash, or discard changes before running this script."
  exit 2
fi

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

start_ref="$(git rev-parse --abbrev-ref HEAD || true)"
if [ "$start_ref" = "HEAD" ] || [ -z "$start_ref" ]; then
  start_ref="$(git rev-parse HEAD)"
fi

# Get back to the original branch when the script exits, even if it fails.
trap 'git checkout --quiet --force "$start_ref"' EXIT

rev_list="$(git rev-list --bisect-all "$base_ref..$head_ref")"
# The most recent commit has already been tested by regular workflow checks.
# If that's the only commit in the PR, we can stop here.
if [[ $(echo "$rev_list" | wc -l) -lt 2 ]]; then
  echo "No previous commits to test."
  exit 0
fi

commits="$(echo "$rev_list" | grep -v 'dist=0' | cut -d' ' -f 1)"
n="$(echo "$commits" | wc -l)"

echo "Testing $n commits:"
echo "$commits" | xargs -I{} git log -1 --pretty='%h %s' {}

failed=""
for commit in $commits; do
  echo "Testing commit $commit"

  # Some files are updated when compiling...
  git checkout --force "$commit"
  git show --stat --oneline

  # Intentionally allow word-splitting for CMAKE_CONFIG to pass multiple -D args.
  cmake -S . -B build ${CMAKE_CONFIG:-}

  cd build
  if ! ( make && make check-unit ); then
    failed="$failed $commit"
  fi
  cd "$repo_root"
done

if [ -n "$failed" ]; then
  echo "Checks failed for these commits:"

  for c in $failed; do
    git show --no-patch --pretty="%h %s (%an, %ad)" "$c"
  done

  exit 1
fi

echo "All previous commits passed checks."

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
