# Startup options

This document explains how to control how AwesomeWM behaves before `rc.lua` is
executed.

## Command line

AwesomeWM has the following command line options:

    Usage: awesome [OPTION]
      -h, --help             show help
      -v, --version          show version
      -c, --config FILE      configuration file to use
      -f  --force            ignore modelines and apply the command line arguments
      -s, --search DIR       add a directory to the library search path
      -k, --check            check configuration file syntax
      -a, --no-argb          disable client transparency support
      -l  --api-level LEVEL  select a different API support level than the current version
      -m, --screen on|off    enable or disable automatic screen creation (default: on)
      -r, --replace          replace an existing window manager

## Modelines

Usually, AwesomeWM is started using a session manager rather than directly using
the command line. On top of that, to make `rc.lua` more portable, it is possible
to set many of those options directly in your config file. They will be
interpreted before the Lua virtual machine is started. The keys are:

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Name</th>
  <th align='center'>Has argument</th>
  <th align='center'>Allow many</th>
  <th align='center'>Type</th>
  <th align='center'>Description</th>
 </tr>
 <tr><td>search</td><td>Yes</td><td>Yes</td><td>string</td><td>Path where the AwesomeWM core libraries are located</td></tr>
 <tr><td>no-argb</td><td>No</td><td>No</td><td>N/A</td><td>Disable built-in (real) transparency.</td></tr>
 <tr><td>api-level</td><td>Yes</td><td>No</td><td>integer</td><td>The config API level.</td></tr>
 <tr><td>screen</td><td>Yes</td><td>No</td><td>string</td><td>Create the screen before executing `rc.lua` (`on` or `off`)</td></tr>
 <tr><td>replace</td><td>No</td><td>No</td><td>N/A</td><td>Replace the current window manager.</td></tr>
</table>

A `modeline` must be near the top of `rc.lua` and start with `-- awesome_mode:`.
The options are separated by `:`. If an option has a value, it is separated by
`=`. The default modeline is:

    -- awesome_mode: api-level=4:screen=on

To display more deprecation errors, you can increase the API level by 2:

    -- awesome_mode: api-level=6:screen=on

## #! (shebang) support.

It is possible to make some `.lua` file executable and use the POSIX magic
prefix (`#!`, often referred as the shebang operator). AwesomeWM will attempt
to parse the header. Note that for now UTF-8 paths are not supported. A typical
file header will look like:

    #! /usr/bin/env awesome --replace
    -- If LuaRocks is installed, make sure that packages installed through it
    -- are found (e.g. lgi). If LuaRocks is not installed, do nothing.
    pcall(require, "luarocks.loader")

    -- Standard awesome library
    local gears = require("gears")
    [... more `rc.lua` content ...]

Then you can make the script executable with `chmod +x` and run it.

## Options description

### version (-v)

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>No</td>
   <td align='center'>No</td></tr>
 </tr>
</table>

The typical output will look like:

    awesome v4.3 (Too long)
      • Compiled against Lua 5.1.5 (running with Lua 5.1)
      • API level: 4
      • D-Bus support: yes
      • xcb-errors support: no
      • execinfo support: yes
      • xcb-randr version: 1.6
      • LGI version: 0.9.2
      • Transparency enabled: yes
      • Custom search paths: no

The content is useful when reporting a bug.

### config (-c): Alternate config path.

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>No</td>
   <td align='center'>No</td></tr>
 </tr>
</table>

This option allows to pass an arbitrary Lua file to be used as a config rather
than `~/.config/awesome/rc.lua` or `/etc/xdg/awesome/rc.lua`. It makes zero
sense in the shebang since you invoke a script directly. It doesn't make sense
in the modeline either since at that point the file is already being read.

### force (-f): Use the command line arguments even when the modeline disagree.

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>No</td>
   <td align='center'>No</td></tr>
 </tr>
</table>

Usually, modelines have the final say of which options to enable. This allows
`rc.lua` to be portable between computers without have to modify `~/.xinitrc`
or your session files. However, sometime, it is useful to override these
parameters. The most common use case is the to bump the API level to see more
deprecation warnings.

### search (-s): Add directories to the Lua search paths.

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
  </tr>
 </tr>
</table>

This option allows to add more paths to the Lua search path. It can be used
to set an alternate version of the core libraries such as `awful` to make
upstream patches development easier. It can also be used to point to custom
modules. It is usually recommended to place custom modules in
`~/.config/awesome/` or `/usr/share/awesome/lib`. Again, this option is mostly
useful for development.

### check (-k): Check the config **SYNTAX**.

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>No</td>
   <td align='center'>No</td></tr>
 </tr>
</table>

This option only check if the file is a valid Lua script. It will not check if
your custom logic makes sense. Even when this option says your config is fine,
it does **NOT** mean it can load properly. It only means it can be parsed and
the interpreter can attempt to execute it.

### no-argb (-a): Mitigate buggy graphics drivers.

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
  </tr>
 </tr>
</table>

This option disables the built-in real transparency support. This means
titlebars and wiboxes can no longer be made fully transparent. If you don't
use a compositing manager such as `compton` or `picom`, this will only improve
reliability and portability. Transparency is enabled by default and this should
only be used when your config misbehave with a popular graphics driver. If it
does, please notify them. The bug is on their side.

### api-level (-l): Force your config to use a different API version.

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
  </tr>
 </tr>
</table>

If you invested a lot of effort into a configuration and a new major version
is AwesomeWM is released, you might want to postpone having to update everything
until you are ready to begin using the new features. If the API level is set,
AwesomeWM will try to honor the behavior and content of the previous APIs.

You can also set this value forward to get notified faster of your usage of
newly deprecated API and enjoy cutting edge experimental features.

The default API level matches the first component of the AwesomeWM version.
For example, AwesomeWM v4.3 API level is "4". This only goes back to AwesomeWM
4.0. The older 3.x APIs have been removed.

### screen (-m): Set the screen creation behavior.

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
  </tr>
 </tr>
</table>

This option changes *when* screen objects are created. In AwesomeWM 4.x, by
default, they are created before `rc.lua` is parsed. This is very convenient
for setup where the number of screen never changes. By creating them before
`rc.lua`, it is possible to make many assumptions such as `mouse.screen` and
`awful.screen.focused()` to always point to a valid screen. This is the `on`
behavior. The main downside is that when the number of screen changes or when
the DPI must be modified, all the automatic magic becomes very inconvenient.

When set to `off`, the screens are created early when executing `rc.lua`. This
has the advantages of sending multiple signals and giving a lot more
configuration options for features like the DPI or ultra-wide monitor. It also
make it easier to add and remove screens dynamically since they are fully. In
the future, the default will be `off` to allow HiDPI support to be enabled by
default.
controllable by Lua code.

### replace (-r): Replace the current window manager with AwesomeWM.

<table class='widget_list' border=1>
 <tr style='font-weight: bold;'>
  <th align='center'>Command line</th>
  <th align='center'>Modeline</th>
  <th align='center'>Shebang</th>
  <tr>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
   <td align='center'>Yes</td>
  </tr>
 </tr>
</table>

If this option is set, AwesomeWM will kill the current window manager (even
if it is another `awesome` instance and replace it. This is disabled by default.
