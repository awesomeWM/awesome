# Tips for upgrading your configuration

<a name="v4"></a>
## From 3.5 to 4.0

The
**best advice is to start from the default `rc.lua` and backport any changes**
you might have added to your `rc.lua`.
This avoids most of the possible errors due to missing important changes.

To do this, you can download the default rc.lua for 3.5.9
[here](https://github.com/awesomeWM/awesome/blob/v3.5.9/awesomerc.lua.in), and
then compare your existing rc.lua with the 3.5.9 default using your diff tool of
choice. Write down the changes, then apply these to the 4.0 default rc.lua,
which you can find at /etc/xdg/awesome/rc.lua after the upgrade, or
[here](../sample%20files/rc.lua.html) if you have not yet performed the upgrade.

If you still wish to ignore this advice, first read the
<a href="89-NEWS.md.html#v4">NEWS</a> section about the breaking changes. This
document assumes you did.

Be warned, even if it looks like it's working after changing some lines, **it wont
supports dynamic screen changes and will have many subtle bugs**. The changes
mentioned below **are important for the stability of your window manager**.

Here is a diff of the 3.5.9 `rc.lua` with the 4.0 one. All changes due to
new features and new syntaxes have been removed. A `-` in front of the line
correspond to content of the 3.5 `rc.lua` and `+` its replacement in 4.0.

This document does not cover the new features added in the Awesome 4 `rc.lua`,
it only covers the minimal required changes to have a properly behaving config.

To test during the port, we recommend the `Xephyr` X11 server.

If Awesome4 **is not installed yet**, we recommand to install it in its own
prefix to avoid conflicts in case you wish to stay on 3.5 for a little
longer:

    cd path/to/awesome/code
    mkdir -p $HOME/.config/awesome4
    mkdir build -p
    cd build

    # Install in a local folder, this assumes all the dependencies are installed
    cmake -DCMAKE_INSTALL_PREFIX=$HOME/awesome4_test ..
    make -j4

    # Use a copy of rc.lua to avoid it being overwritten accidentally
    cp awesomerc.lua $HOME/.config/awesome4/rc.lua

    make install

    # Start Awesome in a 1280x800 window
    Xephyr :1 -screen 1280x800 &
    DISPLAY=:1 $HOME/awesome4_test/bin/awesome \
        -c $HOME/.config/awesome4/rc.lua \
        --search $HOME/awesome4_test/share/awesome/lib

If Awesome 4 is **already installed**, then backup your old `rc.lua` and overwrite
`~/.config/awesome/rc.lua` (replace this by another path if you use a custom
XDH config local directory). And only execute:

    Xephyr :1 -screen 1280x800 &
    DISPLAY=:1 awesome

Screens are now added and removed without reloading `rc.lua`. The wallpaper are
now set in a signal callback.

    --- {{{ Wallpaper
    -if `beautiful.wallpaper` then
    -    for s = 1, screen.count() do
    -        `gears.wallpaper.maximized`(beautiful.wallpaper, s, true)
    -    end
    -end
    --- }}}

    +local function set_wallpaper(s)
    +    -- Wallpaper
    +    if `beautiful.wallpaper` then
    +        local wallpaper = `beautiful.wallpaper`
    +        -- If wallpaper is a function, call it with the screen
    +        if type(wallpaper) == "function" then
    +            wallpaper = wallpaper(s)
    +        end
    +        `gears.wallpaper.maximized`(wallpaper, s, true)
    +    end
    +end
    +
    +-- Re-set wallpaper when a screen's geometry changes (e.g. different resolution)
    +`screen.connect_signal`("property::geometry", set_wallpaper)

Tags need to be created for each screens, the old static initialization cannot
work. Remove this section.

    --- {{{ Tags
    --- Define a tag table which hold all screen tags.
    -tags = {}
    -for s = 1, `screen.count`() do
    -    -- Each screen has its own tag table.
    -    tags[s] = awful.tag({ 1, 2, 3, 4, 5, 6, 7, 8, 9 }, s, layouts[1])
    - end
    --- }}}

The `quit` menu command must be wrapped in a function, otherwise an error
occurs due to mismatched argument types from the v4.0 `awful.menu` library.

     -- {{{ Menu
     -- Create a laucher widget and a main menu
     myawesomemenu = {
       { "manual", terminal .. " -e man awesome" },
       { "edit config", editor_cmd .. " " .. awesome.conffile },
       { "restart", awesome.restart },
    -  { "quit", awesome.quit }
    +  { "quit", function() awesome.quit() end}
     }

The textclock is now part of the `wibox` library, rename it.

     -- {{{ Wibar
     -- Create a textclock widget
    -mytextclock = `awful.widget.textclock`()
    +mytextclock = `wibox.widget.textclock`()


Widgets were previously added to static global tables. This isn't going to
behave correctly when screen are added and removed. Remove this section.

     --- Create a wibox for each screen and add it
     -mywibox = {}
     -mypromptbox = {}
     -mylayoutbox = {}
     -mytaglist = {}
     -mytasklist = {}

Many functions have been converted to methods. The old functions are deprecated,
they are still supported, but will be removed in the next release.

    -mytaglist.buttons = awful.util.table.join(
    +local taglist_buttons = awful.util.table.join(
    -                    awful.button({ }, 1, `awful.tag.viewonly`),
    +                    awful.button({ }, 1, function(t) t:`view_only`() end),
    -                    awful.button({ modkey }, 1, `awful.client.movetotag`),
    +                    awful.button({ modkey }, 1, function(t)
    +                                              if `client.focus` then
    +                                                  `client.focus:move_to_tag`(t)
    +                                              end
    +                                          end),
                         `awful.button`({ }, 3, `awful.tag.viewtoggle`),
    -                    `awful.button`({ modkey }, 3, `awful.client.toggletag`),
    +                    `awful.button`({ modkey }, 3, function(t)
    +                                              if client.focus then
    +                                                  client.focus:toggle_tag(t)
    +                                              end
    +                                          end),
    -                    `awful.button`({ }, 4, function(t) `awful.tag.viewnext`(awful.tag.getscreen(t)) end),
    -                    `awful.button`({ }, 5, function(t) `awful.tag.viewprev`(awful.tag.getscreen(t)) end)
    +                    `awful.button`({ }, 4, function(t) `awful.tag.viewnext`(t.screen) end),
    +                    `awful.button`({ }, 5, function(t) `awful.tag.viewprev`(t.screen) end)
                         )

    -mytasklist.buttons = awful.util.table.join(
    +local tasklist_buttons = awful.util.table.join(
                          awful.button({ }, 1, function (c)
                                                   if c == `client.focus` then
                                                       c.minimized = true
                                                   else
                                                       -- Without this, the following
                                                       -- :isvisible() makes no sense
                                                       c.minimized = false
    -                                                  if not c:isvisible() then
    -                                                      `awful.tag.viewonly`(c:tags()[1])
    +                                                  if not c:isvisible() and c.`first_tag` then
    +                                                      c.first_tag:view_only()
                                                       end
                                                       -- This will also un-minimize
                                                       -- the client, if needed
                                                       `client.focus` = c
                                                       c:raise()
                                                   end
                                               end),


This section is **very important**. This is where adding and removing screens is
handled (including during startup). Note that the `mysomething` table
previously removed are replaced by custom screens attributes.

    -for s = 1, `screen.count`() do
    +`awful.screen.connect_for_each_screen`(function(s)
    +    -- Wallpaper
    +    set_wallpaper(s)
    +
    +    -- Each screen has its own tag table.
    +    awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, s, awful.layout.layouts[1])
    +
         -- Create a promptbox for each screen
    -    mypromptbox[s] = `awful.widget.prompt`()
    +    s.mypromptbox = `awful.widget.prompt`()
         -- Create an imagebox widget which will contains an icon indicating which layout we're using.
         -- We need one layoutbox per screen.
    -    mylayoutbox[s] = `awful.widget.layoutbox`(s)
    +    s.mylayoutbox = `awful.widget.layoutbox`(s)
    -    mylayoutbox[s]:buttons(`awful.util.table.join`(
    +    s.mylayoutbox:buttons(`awful.util.table.join`(
                                `awful.button`({ }, 1, function () `awful.layout.inc`( 1) end),
                                `awful.button`({ }, 3, function () `awful.layout.inc`(-1) end),
                                `awful.button`({ }, 4, function () `awful.layout.inc`( 1) end),
                                `awful.button`({ }, 5, function () `awful.layout.inc`(-1) end)))
         -- Create a taglist widget
    -    mytaglist[s] = `awful.widget.taglist`(s, `awful.widget.taglist.filter.all`, mytaglist.buttons)
    +    s.mytaglist = `awful.widget.taglist`(s, `awful.widget.taglist.filter.all`, taglist_buttons)

         -- Create a tasklist widget
    -    mytasklist[s] = `awful.widget.tasklist`(s, `awful.widget.tasklist.filter.currenttags`, mytasklist.buttons)
    +    s.mytasklist = `awful.widget.tasklist`(s, `awful.widget.tasklist.filter.currenttags`, tasklist_buttons)

         -- Create the wibox
    -    mywibox[s] = `awful.wibox`({ position = "top", screen = s })
    +    s.mywibox = `awful.wibar`({ position = "top", screen = s })

         -- Widgets that are aligned to the left
         local left_layout = wibox.layout.fixed.horizontal()
    -    left_layout:add(mylauncher)
    -    left_layout:add(mytaglist[s])
    -    left_layout:add(mypromptbox[s])
    +    left_layout:add(s.mytaglist)
    +    left_layout:add(s.mypromptbox)

        -- Widgets that are aligned to the right
         local right_layout = wibox.layout.fixed.horizontal()
    -    if s == 1 then right_layout:add(wibox.widget.systray()) end
    +    right_layout:add(wibox.widget.systray())
         right_layout:add(mytextclock)
    -    right_layout:add(mylayoutbox[s])
    +    right_layout:add(s.mylayoutbox)

         -- Now bring it all together (with the tasklist in the middle)
         local layout = wibox.layout.align.horizontal()
         layout:set_left(left_layout)
    -    layout:set_middle(mytasklist[s])
    +    layout:set_middle(s.mytasklist)
         layout:set_right(right_layout)

    -    mywibox[s]:set_widget(layout)
    +    s.mywibox:set_widget(layout)
    end)
     -- }}}


`awful.util.spawn` is now called `awful.spawn`.

    -- Standard program
    -    `awful.key`({ modkey,           }, "Return", function () `awful.util.spawn`(terminal) end),
    +    `awful.key`({ modkey,           }, "Return", function () `awful.spawn`(terminal) end),

Another dynamic screen related changes.

    ⠀-- Prompt
    -    awful.key({ modkey },            "r",     function () mypromptbox[mouse.screen]:run() end),
    +    awful.key({ modkey },            "r",     function () `awful.screen.focused`().mypromptbox:run() end),

`awful.prompt` now uses a more future proof arguments table instead of many
optional arguments.

    ⠀    awful.key({ modkey }, "x",
                   function ()
    -                  `awful.prompt.run`({ prompt = "Run Lua code: " },
    -                  mypromptbox[mouse.screen].widget,
    -                  `awful.util.eval`, nil,
    -                  `awful.util.getdir`("cache") .. "/history_eval")
    -              end),
    +                  awful.prompt.run {
    +                    prompt       = "Run Lua code: ",
    +                    textbox      = `awful.screen.focused`().mypromptbox.widget,
    +                    exe_callback = `awful.util.eval`,
    +                    history_path = `awful.util.get_cache_dir`() .. "/history_eval"
    +                  }
    +              end),


Another function-to-method API change:

    -    `awful.key`({ modkey,           }, "o",      `awful.client.movetoscreen`                        ),
    +    `awful.key`({ modkey,           }, "o",      function (c) c:`move_to_screen`()               end),

The `mod4+[1-9]` keybindings also have some changes related to deprecated
functions.

    -- Bind all key numbers to tags.
    -- Be careful: we use keycodes to make it works on any keyboard layout.
    -- This should map on the top row of your keyboard, usually 1 to 9.
    for i = 1, 9 do
              -- View tag only.
              awful.key({ modkey }, "#" .. i + 9,
                        function ()
     -                        local screen = `mouse.screen`
     -                        local tag = `awful.tag.gettags`(screen)[i]
     +                        local screen = `awful.screen.focused`()
     +                        local tag = `screen.tags`[i]
                              if tag then
     -                           `awful.tag.viewonly`(tag)
     +                           `tag:view_only`()
                              end
                        end),
     -        -- Toggle tag.
     +        -- Toggle tag display.
              awful.key({ modkey, "Control" }, "#" .. i + 9,
                        function ()
     -                      local screen = `mouse.screen`
     -                      local tag = `awful.tag.gettags`(screen)[i]
     +                      local screen = `awful.screen.focused`()
     +                      local tag = `screen.tags`[i]
                            if tag then
                               `awful.tag.viewtoggle`(tag)
                            end
     for i = 1, 9 do
              awful.key({ modkey, "Shift" }, "#" .. i + 9,
                        function ()
                            if client.focus then
     -                          local tag = `awful.tag.gettags`(client.focus.screen)[i]
     +                          local tag = `client.focus.screen.tags`[i]
                                if tag then
     -                              `awful.client.movetotag`(tag)
     +                              `client.focus:move_to_tag`(tag)
                                end
                           end
                        end),
     -        -- Toggle tag.
     +        -- Toggle tag on focused client.
              awful.key({ modkey, "Control", "Shift" }, "#" .. i + 9,
                        function ()
                            if client.focus then
     -                          local tag = `awful.tag.gettags`(client.focus.screen)[i]
     +                          local tag = `client.focus.screen.tags`[i]
                                if tag then
     -                              `awful.client.toggletag`(tag)
     +                              `client.focus:toggle_tag`(tag)
                                end
                            end
     -                  end))
     +                  end),
     +    )
    end

The default rules need to be changed to avoid having offscreen clients:

     awful.rules.rules = {
         -- All clients will match this rule.
         { rule = { },
           properties = { border_width = beautiful.border_width,
               awful.rules.rules = {
                          focus = awful.client.focus.filter,
                          raise = true,
                          keys = clientkeys,
     +                     buttons = clientbuttons,
     +                     screen = `awful.screen.preferred`,
     +                     placement = `awful.placement.no_overlap`+`awful.placement.no_offscreen`

The `tags` global table has been removed to support dynamic screens, you can
now access tags by name.

    -    --   properties = { tag = tags[1][2] } },
    +    --   properties = { screen = 1, tag = "2" } },

If you need to get the current client object in global context, currently you can use
`client.focus` for it. E.g., to mark/unmark the client:

    -    awful.client.mark()
    +    client.focus.marked = true

    -    awful.client.unmark()
    +    client.focus.marked = false
