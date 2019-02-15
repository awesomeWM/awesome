--DOC_GEN_IMAGE
--DOC_NO_USAGE
--DOC_HIDE_ALL
screen[1]._resize {width = 200, height = 100}
screen.no_outline = true
local awful = {tooltip = require("awful.tooltip")}
local beautiful = require("beautiful")
local wibox = require("wibox")

mouse.coords{x=50, y= 10}

    local wb = wibox {width = 100, height = 44, x = 50, y = 25, visible = true}

require("gears.timer").run_delayed_calls_now()
local tt = awful.tooltip {
    text = "A tooltip!",
    objects = {wb},
}
tt.bg = beautiful.bg_normal

mouse.coords{x=75, y= 35}
mouse.push_history()
require("gears.timer").run_delayed_calls_now()
