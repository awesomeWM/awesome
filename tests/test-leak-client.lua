-- Some memory leak checks involving clients as integration tests.
local runner = require("_runner")
local awful = require("awful")
local wibox = require("wibox")
local gtable = require("gears.table")

-- "Enable" titlebars (so that the titlebar can prevent garbage collection)
client.connect_signal("manage", function (c)
    local buttons = gtable.join(
        awful.button({ }, 1, function()
            client.focus = c
            c:raise()
            awful.mouse.client.move(c)
        end),
        awful.button({ }, 3, function()
        client.focus = c
        c:raise()
        awful.mouse.client.resize(c)
    end)
    )

    -- Widgets that are aligned to the left
    local left_layout = wibox.layout.fixed.horizontal(awful.titlebar.widget.iconwidget(c))
    left_layout:buttons(buttons)

    -- The title goes in the middle
    local title = awful.titlebar.widget.titlewidget(c)
    title:set_align("center")
    local middle_layout = wibox.layout.flex.horizontal(title)
    middle_layout:buttons(buttons)

    awful.titlebar(c):set_widget(wibox.layout.align.horizontal(
        left_layout,
        middle_layout,
        wibox.layout.fixed.horizontal(
            awful.titlebar.widget.floatingbutton(c),
            awful.titlebar.widget.maximizedbutton(c),
            awful.titlebar.widget.stickybutton(c),
            awful.titlebar.widget.ontopbutton(c),
            awful.titlebar.widget.closebutton(c)
            )), { position = "bottom"})
end)

-- We tell the garbage collector when to work, disable it
collectgarbage("stop")

-- We try to get a client representing an already-closed client.
-- The first iteration starts xterm, the second keeps a weak reference to it and
-- closes it and the last one checks that the client object is GC'able.
local objs = nil
local second_call = false
local steps = {
    function(count)
        if count == 1 then
            awful.spawn("xterm")
        elseif not objs then
            local c = client.get()[1]
            if c then
                objs = setmetatable({ c }, { __mode = "v" })
                c:kill()
            end
        elseif not second_call then
            -- Wait for one iteration so that gears.timer handles other delayed
            -- calls (= the tasklist updates)
            second_call = true
        elseif #client.get() == 0 then
            assert(#objs == 1)

            -- Test that we have a client and that it's invalid (tostring()
            -- causes an "invalid object" error)
            local success, msg = pcall(function() tostring(objs[1]) end)
            assert(not success)
            assert(msg:find("invalid object"), msg)

            -- Check that it is garbage-collectable
            collectgarbage("collect")
            assert(#objs == 0)
            return true
        end
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
