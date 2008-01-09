#!/bin/sh

# Change to source tree
srcdir=`dirname "$0"`
[ -z "$srcdir" ] || cd "$srcdir"

echo "Generating configure files... may take a while."

autoreconf --install --force && \
  echo "Preparing was successful if there was no error messages above." && \
  echo "Now type:" && \
  echo "  ./configure && make"  && \
  echo "Run './configure --help' for more information"
