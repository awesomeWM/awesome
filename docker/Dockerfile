# Dockerfile for awesomeWM.
#
# ARGS:
#  - DOCKER_FLAVOR: "test" or empty
#  - LUA_PKG: the Lua package to install
#  - LUA_LIB_PREFIX: typically the same as LUA_PKG, but different for luajit
#  - DO_COVERAGE (optional, default: empty): codecov, coveralls, or all
#  - CMAKE_ARGS (optional, default: empty)

FROM alpine:3.6

WORKDIR /src/awesome
ENV HOME /home/awesome

RUN apk --no-cache add --virtual awesome-build \
  cairo-dev cmake dbus-dev gdk-pixbuf-dev git imagemagick \
  libxcb-dev libxdg-basedir-dev libxkbcommon-dev \
  startup-notification-dev xcb-util-cursor-dev xcb-util-keysyms-dev \
  xcb-util-xrm-dev xcb-util-wm-dev \
  gcc make musl-dev \
  libexecinfo-dev

# Also installs some font.
# Not doing so could be used to test https://github.com/awesomeWM/awesome/pull/1782.
RUN apk --no-cache add --virtual awesome-runtime \
  cairo-gobject dbus gdk-pixbuf glib libxcb libxdg-basedir libxkbcommon \
  pango startup-notification xcb-util xcb-util-cursor xcb-util-keysyms \
  xcb-util-wm xcb-util-xrm \
  font-misc-misc \
  libexecinfo

# Install and setup ccache.
# TODO: make this optional; not useful / should not get used/configured on Travis.
RUN apk --no-cache add ccache
ENV PATH /usr/lib/ccache/bin/:$PATH

ARG DOCKER_FLAVOR
RUN if [ "$DOCKER_FLAVOR" = test ]; then \
    apk --no-cache add \
      bash coreutils dbus-x11 grep gtk+3.0 ncurses xprop \
      xrdb xterm xvfb zsh; \
  fi

# Lua, depending on LUA_PKG (can be passed in using --build-arg).
ARG LUA_PKG

RUN apk --no-cache add --virtual awesome-build ${LUA_PKG}-dev
RUN apk --no-cache add --virtual awesome-runtime ${LUA_PKG}
RUN ln -s /usr/bin/${LUA_PKG} /usr/bin/lua

# lua-lgi, special name with luajit.
ARG LUA_LIB_PREFIX
RUN apk --no-cache add --virtual awesome-runtime ${LUA_LIB_PREFIX}-lgi

# Lua test dependencies, required for tests.
RUN if [ "$DOCKER_FLAVOR" = test ]; then \
    apk --no-cache add "${LUA_LIB_PREFIX}-busted"; \
    ln -s /usr/bin/busted-* /usr/bin/busted; \
  fi

ARG DO_COVERAGE=
# NOTE: the lua*-filesystem package gets not picked up by luarocks as a
#       dependency for luacov-coveralls?!
RUN if [ -n "$DO_COVERAGE" ]; then \
    set -ex; \
    apk --no-cache add curl; \
    apk --no-cache add "luarocks${LUA_LIB_PREFIX#lua}"; \
    ln -s /usr/bin/luarocks-${LUA_LIB_PREFIX#lua} /usr/bin/luarocks; \
    luarocks --local install cluacov; \
    if [ "$DO_COVERAGE" = codecov ] || [ "$DO_COVERAGE" = all ]; then \
      curl https://codecov.io/bash > /tmp/codecov-bash; \
    fi; \
    apk --no-cache add --virtual awesome-build python3; \
    if [ "$DO_COVERAGE" = coveralls ] || [ "$DO_COVERAGE" = all ]; then \
      if [ "$LUA_PKG" = luajit ]; then \
        export C_INCLUDE_PATH=/usr/include/luajit-2.1; \
      fi; \
      luarocks install --local luacov-coveralls; \
      python3 -m ensurepip; \
      pip3 --no-cache-dir install cpp-coveralls; \
      pip3 uninstall --yes pip; \
    fi; \
  fi

# Cleanup, needs to happen after build only, and get chained in a single RUN.
# RUN apk del awesome-build

ENV LUA_PKG=${LUA_PKG:-lua5.3}
# For CMake's FindLua.
ENV LUA_DIR /usr/lib/${LUA_PKG}

ARG CMAKE_ARGS=
ENV CMAKE_ARGS=${CMAKE_ARGS}

RUN echo "Built deps image for LUA_PKG=$LUA_PKG"

ENTRYPOINT ["/src/awesome/docker/build-and-test"]
