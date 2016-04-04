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

An example configuration named `awesomerc.lua.in` is provided in the source.

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

Inside gdb you can use the following to print the current Lua stack traceback:

    (gdb) print luaL_dostring(globalconf.L.real_L_dont_use_directly, "print(debug.traceback())")

## Reporting issues

Please report any issues you may find on [our bugtracker](https://github.com/awesomeWM/awesome/issues).
You can submit pull requests on the [github repository](https://github.com/awesomeWM/awesome).
Please read the [contributing guide](https://github.com/awesomeWM/awesome/blob/master/docs/02-contributing.md)
for any coding, documentation or patch guidelines.

## Status
[![Build Status](https://travis-ci.org/awesomeWM/awesome.svg?branch=master)](https://travis-ci.org/awesomeWM/awesome)

## Documentation

Online documentation is available at http://awesome.naquadah.org/doc/ for the
stable branch and at http://awesomewm.github.io/apidoc/ for the master branch.
It can be built using `make ldoc`.

## License

The project is licensed under GNU General Publice License v2 or later.
You can read it online at ([v2](http://www.gnu.org/licenses/gpl-2.0.html)
or [v3](http://www.gnu.org/licenses/gpl.html)).
