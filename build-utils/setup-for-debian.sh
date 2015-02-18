#!/bin/sh
# Install build and test environment on a Debian system.
# This is used from .travis.yml for Travis CI, and is meant to be run
# in a minimal Debian environment (`debootstrap --variant=minbase).

set -e
set -x

# Setup sudo "wrapper" function for "root".
# Especially useful when "sudo" is not installed.
if [ "$(id -u)" = 0 ]; then
  sudo() { "$@"; }
fi

LUA=${1:-5.1}

sudo apt-get update -qq

sudo apt-get install -qq wget
# For luarocks.
sudo apt-get install -qq unzip

sudo apt-get install -qq xvfb

# Based on "apt-get build-depends awesome":
sudo apt-get install -qq cmake imagemagick libcairo2-dev libdbus-1-dev libgdk-pixbuf2.0-dev libglib2.0-dev libstartup-notification0-dev libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-shape0-dev libxcb-util0-dev libxcb-xinerama0-dev libxcb-xtest0-dev libxcb1-dev libxdg-basedir-dev

# Test deps.
sudo apt-get install -qq dbus-x11

# Install Lua.
sudo apt-get install -qq lua$LUA liblua$LUA-dev

# Not necessary for all, maybe when testing generating docs.
#  - sudo apt-get install -qq asciidoc xmlto
# Not available on default Travis image:
# lua-ldoc

# # Not available on default Travis image: libxcb-cursor-dev
# - sudo apt-get install -qq xutils-dev libxcb-render-util0-dev gperf
# - git clone --recursive git://anongit.freedesktop.org/xcb/util-cursor
# - cd util-cursor
# - ./autogen.sh && sudo make install
# - sudo ldconfig
# - cd ..
sudo apt-get install -qq libxcb-cursor-dev

# lgi: too old on travis, install it via luarocks.
#  - sudo apt-get install -qq lua-lgi
sudo apt-get install -qq gir1.2-pango-1.0

# Install luarocks (for the selected Lua version).
wget http://keplerproject.github.io/luarocks/releases/luarocks-2.2.0.tar.gz
tar xf luarocks-2.2.0.tar.gz
cd luarocks-2.2.0
./configure --lua-version=$LUA
sudo make install
cd ..

sudo apt-get install -qq libgirepository1.0-dev
sudo luarocks install lgi

# HACK: script "make check", ref: https://github.com/awesomeWM/awesome/pull/139
# sudo luarocks install busted
