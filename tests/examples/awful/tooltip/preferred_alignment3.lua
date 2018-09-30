--DOC_GEN_IMAGE
--DOC_NO_USAGE
--DOC_HIDE_ALL
screen[1]._resize {width = 300, height = 150}
screen.no_outline = true
local awful = {tooltip = require("awful.tooltip")}
local beautiful = require("beautiful")
local wibox = require("wibox")

-- mouse.coords{x=50, y= 10}

    local wb = wibox {width = 100, height = 44, x = 100, y = 50, visible = true}

awesome.emit_signal("refresh")

for _, side in ipairs{ "left", "right", "bottom", "top" } do
    local tt = awful.tooltip {
        text = side,
        objects = {wb},
        mode = "outside",
        preferred_positions = {side},
        preferred_alignments = {"back"},
    }
    tt.bg = beautiful.bg_normal
end

mouse.coords{x=125, y= 75}
mouse.push_history()
awesome.emit_signal("refresh")
