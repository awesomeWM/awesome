#!/bin/sh

# Change to source tree
srcdir=`dirname "$0"`
[ -z "$srcdir" ] || cd "$srcdir"

# sed program
SED=${SED-sed}

# Check whether the version needs to be updated from VCS/version-stamp
if [ -d ".git" ] && [ -d "autom4te.cache" ]; then
  git_describe=`./build-utils/package-version . version-stamp`
  for f in autom4te.cache/output.*; do
    [ -f "$f" ] || continue
    pkg_ver=`${SED} -n "s/^PACKAGE_VERSION='\(.*\)'\$/\1/p" "$f"`
    if [ "x$pkg_ver" = "x$git_describe" ]
    then :
    else
      echo "Cleaning out autom4te.cache (${pkg_ver} -> ${git_describe})"
      rm -rf "autom4te.cache"
      break
    fi
  done
fi

echo "Generating configure files... may take a while."

autoreconf --install --force && \
  echo "Preparing was successful if there was no error messages above." && \
  echo "Now type:" && \
  echo "  ./configure && make"  && \
  echo "Run './configure --help' for more information"
