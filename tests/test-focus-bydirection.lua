-- Test for https://github.com/awesomeWM/awesome/pull/3225

local runner = require("_runner")
local awful = require("awful")
local beautiful = require("beautiful")

-- Ensure clients are placed next to each other
beautiful.column_count = 3
awful.screen.focused().selected_tag.layout = awful.layout.suit.tile


local steps = {
    function(count)
        if count == 1 then
            awful.spawn("xterm")
            awful.spawn("xterm")
            awful.spawn("xterm")
        else
            local cleft = client.get()[1]
            local cright = client.get()[3]
            client.get()[2].focusable = false

            -- Test with focus.bydirection
            client.focus = cleft
            awful.client.focus.bydirection("right")
            assert(client.focus == cright)

            -- Test with focus.global_bydirection
            client.focus = cleft
            awful.client.focus.global_bydirection("right")
            assert(client.focus == cright)

            return true
        end
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
