# FAQ

## General

### Why call it awesome?

The name *awesome* comes from the English word *awesome* often used by the
character [Barney Stinson](http://en.wikipedia.org/wiki/Barney_Stinson)
from the TV series HIMYM.


## Configuration

### How to change the default window management layout?

In the default configuration file one layout is set for all tags, it happens to
be the `floating` layout. You can change that by editing your tag creation
loop in the `rc.lua`:

    -- Each screen has its own tag table.
    awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, s, awful.layout.layouts[1])

Notice that all tags will use the 1st layout from your layouts table, which is
defined right before tags are created. Just change the layout number in order to
use another window management layout.

#### How to change the name and layout per tag?

You can modify your tag section, there are many possible implementations, here
is a simple one:

At the beginning of `rc.lua`:

    layouts = awful.layout.layouts
    tags = {
      names  = { "www", "editor", "mail", "im", "rss", 6, 7, "rss", "media"},
      layout = { layouts[2], layouts[1], layouts[1], layouts[4], layouts[1],
                 layouts[6], layouts[6], layouts[5], layouts[6]
    }}

Then later to create tags:

    tags[s] = awful.tag(tags.names, s, tags.layout)

#### How to setup different tags and layouts per screen?

Another demonstration for your tag section:

At the beginning of `rc.lua`:

    layouts = awful.layout.layouts
    tags = {
      settings = {
        { names  = { "www", "editor", "mail", "im" },
          layout = { layouts[2], layouts[1], layouts[1], layouts[4] }
        },
        { names  = { "rss",  6, 7,  "media" },
          layout = { layouts[3], layouts[2], layouts[2], layouts[5] }
    }}}

Then later to create tags:

    tags[s] = awful.tag(tags.settings[s.index].names, s, tags.settings[s.index].layout)

#### How to show only non-empty tags?

You can use a filter when creating taglist.

Possible filters:

* `awful.widget.taglist.filter.all` - default, show all tags in taglist
* `awful.widget.taglist.filter.noempty` - show only non-empty tags in taglist (like dynamic tags)
* `awful.widget.taglist.filter.selected` - show only selected tags in taglist

To show only non-empty tags on taglist:

    mytaglist[s] = awful.widget.taglist(s, awful.widget.taglist.filter.noempty, mytaglist.buttons)

### How to autostart applications?

The traditional way is to use the `~/.xinitrc` file.

### How to lock the screen when I am away?

You can use any screen locking utility: `xlock`, `xscreensaver`, `slock`, `kscreenlocker`...

Example key binding for your `globalkeys`:

    awful.key({ modkey }, "F12", function () awful.spawn{ "xlock" } end)

### How to execute a shell command?

If you want to execute a shell command or need to execute a command that uses
redirection, pipes and so on, do not use the `awful.spawn` function but
`awful.util.spawn_with_shell`. Here is an example:

    awful.key({ modkey }, "F10", function () awful.util.spawn_with_shell("cal -m | xmessage -timeout 10 -file -") end)

On zsh, any changes to $PATH you do in `~/.zshrc` will not be picked up (because
this is only run for interactive shells). Use `~/.zshenv` instead to make
additions to the path you want to use in awesome.

If you need to execute command asynchronously (not blocking awesome while long command is running) you can use `awful.spawn.easy_async`.

### How to remove gaps between windows?

You can add `size_hints_honor = false` to the `properties` section in your
`awful.rules.rules` table in your `rc.lua`. It will match and apply this rule
to all clients.

See [the mailing list archive](http://www.mail-archive.com/awesome@naquadah.org/msg01767.html)
for more info about what size hints are.

This might cause flickering with some non-ICCCM conforming applications (e.g.
Lilyterm) which try to override the size that the window manager assigned them.

### How to add even more gaps between windows?

To set gap between clients to 10 px in theme.lua set:

    theme.useless_gap = 10

### How to add an application switcher?

You can use the Clients Menu as an application switcher. By default it will open
if you right-click on your taskbar, but you may also bind it to a key
combination. Here is an example, toggled by "Alt + Esc", that you can add to
your `globalkeys`:

    awful.key({ "Mod1" }, "Escape", function ()
        -- If you want to always position the menu on the same place set coordinates
        awful.menu.menu_keys.down = { "Down", "Alt_L" }
        awful.menu.clients({theme = { width = 250 }}, { keygrabber=true, coords={x=525, y=330} })
    end),


### How to control titlebars?

To disable titlebars on all clients remove the `titlebars_enabled=true` from the
`awful.rules.rules` table in your config. If you want a titlebar only on
certain clients, you can use `awful.rules` to set this property only for certain
clients.

### How to toggle titlebar visibility?

You can use a `clientkeys` binding.:

    awful.key({ modkey, "Shift" }, "t", awful.titlebar.toggle),

### How to toggle wibox visibility?

Add the following key binding to your `globalkeys`:

    awful.key({ modkey }, "b", function ()
        mouse.screen.mywibox.visible = not mouse.screen.mywibox.visible
    end),

### How to toggle clients floating state?

The default `rc.lua` already has a key binding for this, it is "Mod4 + Control +
Space". You can easily change it to something easier like "Mod4 + f" or "Mod4 +
Shift + f".

    awful.key({ modkey, "Shift" }, "f",  awful.client.floating.toggle ),

#### Why some floating clients can not be tiled?

If some of your applications (i.e. Firefox, Opera...) are floating but you can't
tile them, and they behave weird (can not be tagged, are always on top...) do
not panic. They are merely maximized from your last window manager, or from
their last invocation. The default key binding to toggle maximized state is
"Mod4 + m".

You can ensure no application ever starts maximized in the first rule of your
`awful.rules.rules` table, which applies to all clients, by adding:

    -- Search for this rule,
    keys = clientkeys,
    
    -- add the following two:
    maximized_vertical   = false,
    maximized_horizontal = false,

### How to move and resize floaters with the keyboard?

You can use the `client:relative_move` function. The following `clientkeys`
example will move floaters with "Mod4 + Arrow keys" and resize them with "Mod4 +
PgUP/DN" keys:

    awful.key({ modkey }, "Next",  function (c) c:relative_move( 20,  20, -40, -40) end),
    awful.key({ modkey }, "Prior", function (c) c:relative_move(-20, -20,  40,  40) end),
    awful.key({ modkey }, "Down",  function (c) c:relative_move(  0,  20,   0,   0) end),
    awful.key({ modkey }, "Up",    function (c) c:relative_move(  0, -20,   0,   0) end),
    awful.key({ modkey }, "Left",  function (c) c:relative_move(-20,   0,   0,   0) end),
    awful.key({ modkey }, "Right", function (c) c:relative_move( 20,   0,   0,   0) end),

#### How to resize tiled clients?

You can use the `awful.tag.incmwfact` function to resize master clients and
`awful.client.incwfact` function to resize slave clients. The following
`globalkeys` example demonstrates this:

    awful.key({ modkey }, "l",          function () awful.tag.incmwfact( 0.05) end),
    awful.key({ modkey }, "h",          function () awful.tag.incmwfact(-0.05) end),
    awful.key({ modkey, "Shift" }, "l", function () awful.client.incwfact(-0.05) end),
    awful.key({ modkey, "Shift" }, "h", function () awful.client.incwfact( 0.05) end),

### How to change awesome configuration while it's running?

You can modify `rc.lua`, but you have to restart awesome for changes to take
effect. The default keybinding for restarting awesome is "Mod4 + Control + r".

### How to find window's class and other identifiers?

You can use the `xprop` utility, you are interested in `WM_CLASS` and `WM_NAME`
from its output:

  $ xprop WM_CLASS WM_NAME

When the cursor changes to "+" click on the client of interest. From the
terminal output you can use the following to match clients in awesome:

    WM_CLASS(STRING) = "smplayer", "Smplayer"
                        |           |
                        |           |--- class
                        |
                        |--- instance
    
    WM_NAME(STRING) = "SMPlayer"
                       |
                       |--- name

You can use the above identifiers (instance, class and name) in your
`awful.rules.rules` table to do matching, tagging and other client manipulation.
See the next FAQ answer for some examples.

### How to start clients on specific tags and others as floating?

You can add matching rules to your `awful.rules.rules` table. The default
`rc.lua` already has several examples, but some more can be found in the
@{awful.rules.rules|documentation}.

### How to start clients as slave windows instead of master?

You can set windows to open as slave windows by setting rule to match all
clients:

    -- Start windows as slave
    { rule = { }, properties = { }, callback = awful.client.setslave }

### How to use a keycode in a keybinding?

You can use the format `#XYZ` for keycodes in your bindings. The following
example shows a mapped multimedia/extra key, that's why the modifier is not
present (but it could be):

    awful.key({}, "#160", function () awful.spawn("kscreenlocker --forcelock") end),

### How to add a keyboard layout switcher?

The `wibox.widget.keyboardlayout` is a widget that shows the current keyboard
layout and allows to change it by clicking on it.

### How to make windows spawn under the mouse cursor?

In the default `awful.rules`-rule, the following placement is specified:

    placement = awful.placement.no_overlap+awful.placement.no_offscreen

You can prepend `awful.placement.under_mouse` to this:

    placement = awful.placement.under_mouse+awful.placement.no_overlap+awful.placement.no_offscreen

### How to switch to a specific layout in a keybinding?

You can call the `awful.layout.set()` function, here's an example:

    awful.key({ modkey }, "q", function () awful.layout.set(awful.layout.suit.tile) end),

### Why are new clients urgent by default?

You can change this by redefining `awful.ewmh.activate(c)` in your rc.lua. If
you don't want new clients to be urgent by default put this in your rc.lua:

    client.disconnect_signal("request::activate", awful.ewmh.activate)
    function awful.ewmh.activate(c)
        if c:isvisible() then
            client.focus = c
            c:raise()
        end
    end
    client.connect_signal("request::activate", awful.ewmh.activate)
    
### How to prevent mouse cursor from jumping to the corner when resizing or moving a client

You can specify the behavior for these layouts:

    awful.layout.suit.floating.resize_jump_to_corner = false
    awful.layout.suit.tile.resize_jump_to_corner = false
    
### How to create a simple panel widget

See an example in the documentation for `awful.widget.watch`.


## Usage

### How to use this thing?

Default binding to open a terminal is "Mod4 + Enter" (where Mod4 is usually the
"Windows" key). You can also click on the desktop background with the right
button, to open the awesome menu.

From there you can proceed to open `man awesome` which has a good guide,
including the list of default keybindings.

The list of currently enabled keybindings can be opened by "Mod4 + s" or from the awesome menu.

### Layouts

With the default config, you can cycle through window layouts by pressing
"mod4+space" ("mod4+shift+space" to go back) or clicking the layout button in
the upper right corner of the screen.

### How to restart or quit awesome?

You can use the keybinding "Mod4+Ctrl+r" or by selecting restart in the menu.
You could call `awesome.restart` either from the Lua prompt widget, or via
`awesome-client`:

    $ awesome-client 'awesome.restart()'

You can also send the `SIGHUP` signal to the awesome process. Find the PID using
`ps`, `pgrep` or use `pkill`:

  $ pkill -HUP awesome

You can quit awesome by using "Mod4+Shift+q" keybinding or by selecting quit in
the menu. You could call `awesome.quit` either from the Lua prompt widget,
or by passing it to `awesome-client`.

    $ echo 'awesome.quit()' | awesome-client

You can also send the `SIGINT` signal to the awesome process. Find the PID using `ps`, `pgrep` or use `pkill`:

    $ pkill -INT awesome

### Why awesome doesn't use my own brand new config?

If awesome cannot find `$XDG_CONFIG_HOME/awesome/rc.lua`, or fails to load it,
it falls back to using `/etc/xdg/awesome/rc.lua` (you haven't edited it, I hope,
have you?). Even if `awesome --check` hasn't reported any error, it only means
that your `rc.lua` is syntactically correct, but absence of runtime errors is
not guaranteed. Moreover, awesome could apply half of your config then encounter
an error and load stock one, and that could lead to bizzare result, like two
sets of tags. See the next entry on how to find out where the problem lurks.

### Where are logs, error messages or something?

When hacking your own configuration, something inevitably would go wrong.
awesome prints error messages to its `stderr` stream. When run with usual `$
startx`, it'd be printed right in tty. If you use something more complicated
(some kind of DM, like kdm or gdm), stderr is usually redirected somewhere else.
To see where, run the following command:

    $ ls -l /proc/$(pidof awesome)/fd/2

There's handy way to run awesome and redirect both its standard output and error streams to files:

    exec /usr/bin/awesome >> ~/.cache/awesome/stdout 2>> ~/.cache/awesome/stderr

If you put it into `.xinitrc` (for `startx`) or `~/.xsession`, you'll be able to
watch (with `tail -f`) everything right from awesome.

### Why does Mod4 "swallow" succeeding key presses?

On some systems xkb by default maps the left windows key to "Multi_key" (at
least in `us` and `de` layouts). Multi_key is an xkb feature which may be used
to access uncommon symbols by pressing Multi_key and then (consecutively) two
"normal" keys. The solution is to remap your windows key to mod4 and remove the
Multi_key mapping. This can be done by including "altwin(left_meta_win)" in the
xkb keyboard description xkb_symbols line.

    #!/bin/bash
    xkbcomp - $DISPLAY<<EOF
    xkb_keymap {
    xkb_keycodes  { include "evdev+aliases(qwertz)"};
    xkb_types     { include "complete"};
    xkb_compat    { include "complete"};
    xkb_symbols   { include "pc+de(nodeadkeys)+inet(evdev)+group(alt_shift_toggle)+level3(ralt_switch)+altwin(left_meta_win)+capslock(escape)"    };
    xkb_geometry  { include "pc(pc104)"};
    };
    EOF

### I upgraded from Awesome 3 to Awesome 4 and multiscreen broke. Why is that?

Awesome 4.0+ support dynamic screen plugging and unplugging without restarting.

This avoids losing your tags, layout and focus history. Olders `rc.lua` were not
designed to support such changes and assumed Awesome would restart. To add
multi-screen support to existing configs, see how
`awful.screen.connect_for_each_screen` is used in the new `rc.lua` or rebuild
your config on a newer revision of `rc.lua`.

### Can I have a client or the system tray on multiple screens at once?

No. This is an X11 limitation and there is no sane way to work around it.

### Can a client be tagged on different screens at once?

While it is not impossible to partially implement support for this, many
Awesome components frequently query the client's screen.
Since a client can only be in one screen at once, this will cause side effects.
So by default Awesome avoids, but does not prevent, having clients in multiple
tags that are not on the same screen.

### Can a tag be on multiple screens?

No. See the previous two questions. However, it is possible to swap tags across
screens using `t:swap(t2)` (assuming `t` and `t2` are valid `tag` objects).

This can be used to emulate a tag being on multiple screens. Note that this will
break support for multi-tagged clients. For this reason it isn't implemented by
default.

## Development

### How to report bugs?

First, test the development version to check if your bug is still there. If the
bug is an unexpected behavior, please explain what you expected instead. If the
bug is a segmentation fault, please include a full backtrace (use gdb).

In any case, please try to explain how to reproduce it.

Please report any issues you may find on [our
bugtracker](https://github.com/awesomeWM/awesome/issues).

### Do you accept patches and enhancements?

Yes, we do.
You can submit pull requests on the [github repository](https://github.com/awesomeWM/awesome).
Please read the [contributing guide](https://github.com/awesomeWM/awesome/blob/master/docs/02-contributing.md)
for any coding, documentation or patch guidelines.
