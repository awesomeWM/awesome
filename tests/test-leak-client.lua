-- Some memory leak checks involving clients as integration tests.
local runner = require("_runner")
local awful = require("awful")
local wibox = require("wibox")
local gtable = require("gears.table")

-- Create a titlebar and return a table with references to its member widgets.
local function create_titlebar(c)
    local parts = {}
    local buttons = {
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
    }

    -- Widgets that are aligned to the left
    parts.icon = awful.titlebar.widget.iconwidget(c)
    local left_layout = wibox.layout.fixed.horizontal(parts.icon)
    left_layout.buttons = buttons

    -- The title goes in the middle
    parts.title = awful.titlebar.widget.titlewidget(c)
    parts.title:set_align("center")
    local middle_layout = wibox.layout.flex.horizontal(parts.title)
    middle_layout.buttons = buttons

    parts.floating_button  = awful.titlebar.widget.floatingbutton(c)
    parts.maximized_button = awful.titlebar.widget.maximizedbutton(c)
    parts.sticky_button    = awful.titlebar.widget.stickybutton(c)
    parts.ontop_button     = awful.titlebar.widget.ontopbutton(c)
    parts.close_button     = awful.titlebar.widget.closebutton(c)

    awful.titlebar(c):set_widget(wibox.layout.align.horizontal(
        left_layout,
        middle_layout,
        wibox.layout.fixed.horizontal(
            parts.floating_button,
            parts.maximized_button,
            parts.sticky_button,
            parts.ontop_button,
            parts.close_button
            )), { position = "bottom"})

    return parts
end

-- "Enable" titlebars (so that the titlebar can prevent garbage collection)
client.connect_signal("request::manage", function (c)
    create_titlebar(c)
end)

-- We tell the garbage collector when to work, disable it
collectgarbage("stop")

-- We try to get a client representing an already-closed client.
-- The first iteration starts xterm, the second keeps a weak reference to it and
-- closes it and the last one checks that the client object is GC'able.
local objs = nil
local second_call = false
local state
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
            assert(not success, msg)
            assert(msg:find("invalid object"), msg)

            -- Check that it is garbage-collectable
            collectgarbage("collect")

            -- On GitHub Actions, it can take a while for clients to be killed
            -- properly.
            local tries = 0
            while (#objs > 0 and tries < 60) do
                os.execute("sleep 1")
                tries = tries + 1
            end

            if tries > 0 then
                print("Took approx. " .. tries .. " seconds to clean leaked client")
            end

            assert(#objs == 0, "still clients left after garbage collect")
            return true
        end
    end,

    -- Test that titlebar widgets are GC'able even when the corresponding client
    -- still exists.
    function(count)
        if count == 1 then
            awful.spawn("xterm")
            state = 1
        elseif state == 1 then
            local c = client.get()[1]
            if c then
                -- Create the titlebar and store references to its widgets in a
                -- weak table.
                objs = setmetatable({}, { __mode = "v" })
                gtable.crush(objs, create_titlebar(c))
                state = 2
            end
        elseif state == 2 then
            local c = client.get()[1]

            -- Recreate the titlebar; at this point old titlebar widgets should
            -- become unreferenced, except for some delayed calls.
            create_titlebar(c)

            -- Wait for one iteration so that delayed calls would complete and
            -- release remaining references to widgets.
            state = 3
        elseif state == 3 then
            -- Check that old titlebar widgets have been destroyed.
            for _ = 1, 10 do
                collectgarbage("collect")
            end
            local num_objs = 0
            for k in pairs(objs) do
                if num_objs == 0 then
                    print("Error: titlebar widgets remaining after GC:")
                end
                num_objs = num_objs + 1
                print(k)
            end
            assert(num_objs == 0)

            client.get()[1]:kill()
            state = 4
        elseif state == 4 then
            if #client.get() == 0 then
                return true
            end
        end
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
