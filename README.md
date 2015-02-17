awesome
=======
awesome is a highly configurable, next generation framework window manager for X.

Building and installation
-------------------------
After extracting the dist tarball, run:

    make

This will create a build directory, run cmake in it and build awesome.

After building is finished, you can install:

    make install  # you might need root permissions

Running awesome
---------------
You can directly select awesome from your display manager. If not, you can
add the following line to your .xinitrc to start awesome using startx
or to .xsession to start awesome using your display manager:

    exec awesome

In order to connect awesome to a specific display, make sure that
the DISPLAY environment variable is set correctly, e.g.:

    DISPLAY=foo.bar:1 exec awesome

(This will start awesome on display :1 of the host foo.bar.)

Configuration
-------------
The configuration of awesome is done by creating a
`$XDG_CONFIG_HOME/awesome/rc.lua` file.

An example configuration named "awesomerc.lua.in" is provided in the source.

Troubleshooting
---------------
In most systems any message printed by awesome (including warnings and errors)
are written to `$HOME/.xsession-errors`.

If awesome does not start or the configuration file is not producing the
desired results the user should examine this file to gain insight into the
problem.

Reporting issues
----------------
Please report any issues you may find at https://github.com/awesomeWM/awesome/issues.
You can submit pull requests at https://github.com/awesomeWM/awesome.
