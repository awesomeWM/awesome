# Readme

## About awesome

awesome is a highly configurable, next generation framework window manager for X.

## Building and installation

After extracting the dist tarball, run:

    make

This will create a build directory, run cmake in it and build awesome.

After building is finished, you can either install via `make install`:

    make install  # you might need root permissions

or by auto-generating a .deb or .rpm package, for easy removal later on:

    make package

    sudo dpkg -i awesome-x.y.z.deb
    # or
    sudo rpm -Uvh awesome-x.y.z.rpm

NOTE: awesome uses [`cmake`](https://cmake.org) to build. In case you want to
pass arguments to cmake, please use the `CMAKE_ARGS` environment variable. For
instance:

    CMAKE_ARGS="-DCMAKE_INSTALL_PREFIX=/opt/awesome" make

### Build dependencies

Awesome has the following dependencies (besides a more-or-less standard POSIX
environment):

- [CMake >= 3.0.0](https://cmake.org)
- [Lua >= 5.1.0](https://www.lua.org) or [LuaJIT](http://luajit.org)
- [LGI >= 0.8.0](https://github.com/pavouk/lgi)
- [xproto >= 7.0.15](https://www.x.org/archive//individual/proto/)
- [libxcb >= 1.6](https://xcb.freedesktop.org/) with support for the RandR, XTest, Xinerama, SHAPE and
  XKB extensions
- [libxcb-cursor](https://xcb.freedesktop.org/)
- [libxcb-util >= 0.3.8](https://xcb.freedesktop.org/)
- [libxcb-keysyms >= 0.3.4](https://xcb.freedesktop.org/)
- [libxcb-icccm >= 0.3.8](https://xcb.freedesktop.org/)
- [xcb-util-xrm >= 1.0](https://github.com/Airblader/xcb-util-xrm)
- [libxkbcommon](http://xkbcommon.org/) with X11 support enabled
- [libstartup-notification >=
  0.10](https://www.freedesktop.org/wiki/Software/startup-notification/)
- [cairo](https://www.cairographics.org/) with support for XCB and GObject
  introspection
- [Pango](http://www.pango.org/) with support for Cairo and GObject
  introspection
- [GLib >= 2.40](https://wiki.gnome.org/Projects/GLib) with support for GObject
  introspection
- [GIO](https://developer.gnome.org/gio/stable/) with support for GObject
  introspection
- [GdkPixbuf](https://wiki.gnome.org/Projects/GdkPixbuf)
- libX11 with xcb support
- [Imagemagick's convert utility](http://www.imagemagick.org/script/index.php)
- [libxdg-basedir >= 1.0.0](https://github.com/devnev/libxdg-basedir)

Additionally, the following optional dependencies exist:

- [DBus](https://www.freedesktop.org/wiki/Software/dbus/) for DBus integration
  and the `awesome-client` utility
- [asciidoc](http://www.methods.co.nz/asciidoc/) and
  [xmlto](https://fedorahosted.org/xmlto/) for generating man pages
- [gzip](http://www.gzip.org/) for compressing man pages
- [ldoc](https://stevedonovan.github.io/ldoc/) for generating the documentation
- [busted](https://olivinelabs.com/busted/) for running unit tests
- [luacheck](https://github.com/mpeterv/luacheck) for static code analysis
- [LuaCov](https://keplerproject.github.io/luacov/) for collecting code coverage
  information
- libexecinfo on systems where libc does not provide `backtrace_symbols()` to
  generate slightly better backtraces on crashes
- `Xephyr` or `Xvfb` for running integration tests

## Running awesome

You can directly select awesome from your display manager. If not, you can
add the following line to your .xinitrc to start awesome using startx
or to `.xsession` to start awesome using your display manager:

    exec awesome

In order to connect awesome to a specific display, make sure that
the `DISPLAY` environment variable is set correctly, e.g.:

    DISPLAY=foo.bar:1 exec awesome

(This will start awesome on display `:1` of the host foo.bar.)

## Configuration

The configuration of awesome is done by creating a
`$XDG_CONFIG_HOME/awesome/rc.lua` file, typically `~/.config/awesome/rc.lua`.

An example configuration named `awesomerc.lua` is provided in the source.

## Troubleshooting

On most systems any message printed by awesome (including warnings and errors)
is written to `~/.xsession-errors`.

If awesome does not start or the configuration file is not producing the
desired results the user should examine this file to gain insight into the
problem.

### Debugging tips

You can call `awesome` with `gdb` like this:

    DISPLAY=:2 gdb awesome

Then in gdb set any args and run it:

    (gdb) set arg --replace
    (gdb) run

## Reporting issues

Please report any issues you may find on [our bugtracker](https://github.com/awesomeWM/awesome/issues).
You can submit pull requests on the [github repository](https://github.com/awesomeWM/awesome).
Please read the [contributing guide](https://github.com/awesomeWM/awesome/blob/master/docs/02-contributing.md)
for any coding, documentation or patch guidelines.

## Status
[![Build Status](https://travis-ci.org/awesomeWM/awesome.svg?branch=master)](https://travis-ci.org/awesomeWM/awesome)

## Documentation

Online documentation is available at https://awesomewm.org/apidoc/.

## License

The project is licensed under GNU General Publice License v2 or later.
You can read it online at ([v2](http://www.gnu.org/licenses/gpl-2.0.html)
or [v3](http://www.gnu.org/licenses/gpl.html)).
