#!/usr/bin/env bash
#
# Process (API) docs after a successful build on Travis (via ../.travis.yml).
#
# Updated/changed documentation for "master" is pushed to gh-pages.
# In case of pull requests or other branches, it will get added to a separate branch.
# In case of a pull request, a compare view comment will be posted.
#
# NOTE: stdout/stderr might/should be discarded to not leak sensitive information.

echo "Post-processing (API) documentation."
echo "TRAVIS_PULL_REQUEST: $TRAVIS_PULL_REQUEST"
echo "TRAVIS_BRANCH: $TRAVIS_BRANCH"

if [ -z "$GH_APIDOC_TOKEN" ]; then
  echo "No GH_APIDOC_TOKEN available. Skipping."
  exit
fi

# NOTE: DO NOT USE "set -x", or anything else that would reveal GH_APIDOC_TOKEN!
set -e
set +x

# Display exit code in term of failure (probably due to 'set -x').
trap '[ "$?" = 0 ] || echo "EXIT CODE: $?"' EXIT

REPO_APIDOC="https://${GH_APIDOC_TOKEN}@github.com/awesomeWM/apidoc"
REPO_DIR="$PWD"

export GIT_AUTHOR_NAME="awesome-robot on Travis CI"
export GIT_AUTHOR_EMAIL="awesome-robot@users.noreply.github.com"
export GIT_COMMITTER_NAME="$GIT_AUTHOR_NAME"
export GIT_COMMITTER_EMAIL="$GIT_AUTHOR_EMAIL"

git clone --depth 1 --branch gh-pages "$REPO_APIDOC" build/apidoc \
    2>&1 | sed "s/$GH_APIDOC_TOKEN/GH_APIDOC_TOKEN/g"
cd build/apidoc

# This will re-use already existing branches (updated PR).
if [ "$TRAVIS_PULL_REQUEST" != false ]; then
  BRANCH="pr-$TRAVIS_PULL_REQUEST"
elif [ "$TRAVIS_BRANCH" != master ]; then
  # Use merge-base of master in branch name, to keep different branches with
  # the same name apart.
  # shellcheck disable=SC2015
  BRANCH="$TRAVIS_BRANCH-$(cd "$REPO_DIR" \
    && git fetch --unshallow origin master \
    && git rev-parse --short "$(git merge-base HEAD FETCH_HEAD || true)" || true)"
else
  BRANCH="gh-pages"
fi
if [ "$BRANCH" != "gh-pages" ]; then
  git checkout -b "$BRANCH" "origin/${BRANCH}" || git checkout -b "$BRANCH"
fi

# Use a temporary branch for the two commits, which allows for a better UI.
git checkout -b merged-update

# Create the README for the Git repo (https://github.com/awesomeWM/apidoc).
cat > ../doc/README.md <<END
# Awesome API documentation

This repository contains the built API documentation for the
[awesome](https://github.com/awesomeWM/awesome) window manager. It is
automatically updated via Travis when the master branch changes. Hence:

## Do NOT send pull requests here

Instead, please update the source code of
[awesome](https://github.com/awesomeWM/awesome) instead.
END

# Create a patch without irrelevant changes (version / timestamp).
diff -Nur . ../doc -I "Last updated" -I "<strong>Release</strong>:" \
  -I "<h2>API documentation for awesome, a highly configurable X window manager (version .*)\.</h2>" \
  -x .git | patch -p1

git add --all .
if git diff --cached --exit-code --quiet; then
  echo "Documentation has not changed."
  exit
fi

LAST_COMMIT_MSG="$(cd "$REPO_DIR" && git log -1 --pretty=format:%s)"
LAST_COMMIT="$(cd "$REPO_DIR" && git rev-parse --short HEAD)"

# Commit the relevant changes.
COMMIT_MSG="Update docs for $AWESOME_VERSION via Travis

Last commit message:
$LAST_COMMIT_MSG

Commits: https://github.com/awesomeWM/awesome/compare/${TRAVIS_COMMIT_RANGE/.../..}
Build URL: https://travis-ci.com/awesomeWM/awesome/builds/${TRAVIS_BUILD_ID}"
git commit -m "[relevant] $COMMIT_MSG"

# Commit the irrelevant changes.
mv .git ../doc
cd ../doc
git add --all .
git commit -m "[boilerplate] $COMMIT_MSG"

# Reorder/swap commits, to have "relevant" after "boilerplate".
# This makes it show up earlier in the Github interface etc.
git tag _old
git reset --hard HEAD~2
git cherry-pick _old _old~1
RELEVANT_REV="$(git rev-parse --short HEAD)"
git tag -d _old

git checkout "$BRANCH"
OLD_REV="$(git rev-parse --short HEAD)"
if [ "$TRAVIS_PULL_REQUEST" != false ]; then
    MERGE_COMMIT_MSG="$COMMIT_MSG
Pull request: https://github.com/awesomeWM/awesome/pull/${TRAVIS_PULL_REQUEST}"
else
    PR_OR_ISSUE="$(echo "$COMMIT_MSG" | head -n 1 | grep -o '#[0-9]\+' || true)"
    if [ -n "$PR_OR_ISSUE" ]; then
        MERGE_COMMIT_MSG="$COMMIT_MSG
Ref: https://github.com/awesomeWM/awesome/pull/${PR_OR_ISSUE}"
    else
        PR_OR_ISSUE_URL="$(echo "$COMMIT_MSG" \
            | grep -Eo 'https://github.com/awesomeWM/awesome/(issues|pull)/[0-9]+' || true)"
        if [ -n "$PR_OR_ISSUE_URL" ]; then
            MERGE_COMMIT_MSG="$COMMIT_MSG
Ref: $PR_OR_ISSUE_URL"
        else
            MERGE_COMMIT_MSG="$COMMIT_MSG
Commit: https://github.com/awesomeWM/awesome/commit/${LAST_COMMIT}
Tree:   https://github.com/awesomeWM/awesome/commits/${LAST_COMMIT}"
        fi
    fi
fi
git merge --no-ff -m "$MERGE_COMMIT_MSG" merged-update
NEW_REV="$(git rev-parse --short HEAD)"

git push origin "$BRANCH" 2>&1 | sed "s/$GH_APIDOC_TOKEN/GH_APIDOC_TOKEN/g"

# Generate compare view links.
# NOTE: use "\n" for line endings, not real ones for valid json!
COMPARE_LINKS="Compare view: https://github.com/awesomeWM/apidoc/compare/${OLD_REV}...${NEW_REV}"
COMPARE_LINKS="$COMPARE_LINKS\nRelevant changes: https://github.com/awesomeWM/apidoc/commit/${RELEVANT_REV}"
if [ "$BRANCH" != "gh-pages" ]; then
  COMPARE_LINKS="$COMPARE_LINKS\nComparison against master (gh-pages): https://github.com/awesomeWM/apidoc/compare/gh-pages...${NEW_REV}"
fi
# shellcheck disable=SC2028
echo "Compare links:\n$COMPARE_LINKS"

# Post a comment to the PR.
if [ "$TRAVIS_PULL_REQUEST" != false ]; then
  curl -H "Authorization: token $GH_APIDOC_TOKEN" \
    -d "{\"body\": \"Documentation has been updated for this PR.\n\n$COMPARE_LINKS\"}" \
    "https://api.github.com/repos/awesomeWM/awesome/issues/${TRAVIS_PULL_REQUEST}/comments" \
    2>&1 | sed "s/$GH_APIDOC_TOKEN/GH_APIDOC_TOKEN/g"
fi

# vim: filetype=sh:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
