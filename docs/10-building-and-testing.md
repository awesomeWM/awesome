# Building and Testing

## Dependencies

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
- [libxcb-xfixes](https://xcb.freedesktop.org/)
- [libxcb-composite](https://xcb.freedesktop.org/)
- [libxcb-damage](https://xcb.freedesktop.org/)
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
- [asciidoctor](https://asciidoctor.org/) for generating man pages
- [gzip](http://www.gzip.org/) for compressing man pages
- [ldoc >= 1.4.5](https://stevedonovan.github.io/ldoc/) for generating the
  documentation
- [busted](https://olivinelabs.com/busted/) for running unit tests
- [luacheck](https://github.com/mpeterv/luacheck) for static code analysis
- [LuaCov](https://keplerproject.github.io/luacov/) for collecting code coverage
  information
- libexecinfo on systems where libc does not provide `backtrace_symbols()` to
  generate slightly better backtraces on crashes
- `Xephyr` or `Xvfb` for running integration tests
- [GTK+ >= 3.10](https://www.gtk.org/) for `./themes/gtk/`
- [xcb-errors](https://gitlab.freedesktop.org/xorg/lib/libxcb-errors) for
  pretty-printing of X11 errors
- [libRSVG](https://wiki.gnome.org/action/show/Projects/LibRsvg) for displaying
  SVG files without scaling artifacts
- [wmctrl](http://tripie.sweb.cz/utils/wmctrl) for testing WM interactions
  with external actions
- [xterm](https://invisible-island.net/xterm/) for various test cases

## Building

With all dependencies installed, run the following commands within the root of the repository to build and install Awesome on your system:

```sh
make
sudo make install
```

### Create packages

To ease installation management, you can pack Awesome into `.deb` or `.rpm` packages. The following command will build the correct package for your system's package manager:

```sh
make package
```

To install the package, run one of the following commands:

```sh
sudo dpkg -i awesome-x.y.z.deb
# or
sudo rpm -Uvh awesome-x.y.z.rpm
```

### Customizing build options

Awesome allows customizing various aspects of the build process through CMake variables. To change these variables, pass the `CMAKE_ARGS` environment variable to `make` with CMake flags as content (see `cmake -h`).

Here is an example:

```sh
CMAKE_ARGS="-DLUA_EXECUTABLE=/usr/bin/lua5.3" make
```

**Note:**

CMake arguments only need to be specified on the first run of `make` or after `make distclean`.

#### Skip documentation

The CMake variables `GENERATE_DOC` and `GENERATE_MANPAGES` toggle generation of HTML documentation and man pages respectively. They are enabled by default but can be set to `OFF`.

```sh
CMAKE_ARGS="-DGENERATE_DOC=OFF -DGENERATE_MANPAGES=OFF" make
```

#### Change the Lua version

Both the build steps and the final binary require a working Lua interpreter. To change the Lua interpreter that both the build process and Awesome should use, specify an absolute path to a `lua` binary for `LUA_EXECUTABLE` and values that match the binary's version for `LUA_LIBRARY` and `LUA_INCLUDE_DIR`.

Example to build on Arch Linux:

```sh
CMAKE_ARGS="-DLUA_EXECUTABLE=/usr/bin/lua5.3 -DLUA_LIBRARY=/usr/lib/liblua.so.5.3 -DLUA_INCLUDE_DIR=/usr/include/lua5.3" make
```

#### Additional options

Additional variables can be found through CMake. Run the following command to print a list of all available variables, including the ones explained above:

```sh
cmake -B build -L
```

## Testing

Once Awesome has been built, you can run various test suites to verify functionality for both the binary and the Lua library. Make sure to install applicable dependencies as outlined at the top.

**Full test suite:**

```sh
make check
```

Individual test categories can be run as well:

* `make check-integration`: Run integration tests within a Xephyr session.
* `make check-qa`: Run `luacheck` against the Lua library
* `make check-unit`: Run unit tests with `busted` against the Lua library. You can also run `busted <options> ./spec` if you want to specify options for `busted`.
* `make check-requires`: Check for invalid `require()` calls.
* `make check-examples`: Run integration tests within the examples in `./tests/examples`.
* `make check-themes`: Test themes.
