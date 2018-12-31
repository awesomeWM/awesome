--DOC_GEN_IMAGE
--DOC_NO_USAGE
--DOC_HIDE_ALL
screen[1]._resize {width = 640, height = 55}
screen.no_outline = true
local awful = {tooltip = require("awful.tooltip")}
local beautiful = require("beautiful")
local gears = {shape = require("gears.shape")}

-- mouse.coords{x=50, y= 10}

awesome.emit_signal("refresh")

local x_offset = 0

for _, shape in ipairs{ "rounded_rect", "rounded_bar", "octogon", "infobubble"} do
    local tt = awful.tooltip {
        text = shape,
        mode = "mouse",
        border_width = 2,
        shape = gears.shape[shape],
        border_color = beautiful.border_color,
    }
    tt.bg = beautiful.bg_normal
    awesome.emit_signal("refresh")
    tt:show()
    awesome.emit_signal("refresh")
    tt.wibox.x = x_offset
    x_offset = x_offset + 640/5
end

mouse.coords{x=125, y= 0}
-- mouse.push_history()
awesome.emit_signal("refresh")
