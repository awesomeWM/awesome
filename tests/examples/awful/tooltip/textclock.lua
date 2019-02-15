--DOC_GEN_IMAGE
--DOC_NO_USAGE
require("_date") --DOC_HIDE
screen[1]._resize {width = 300, height = 75} --DOC_HIDE
local awful = {tooltip = require("awful.tooltip"), wibar = require("awful.wibar")} --DOC_HIDE
local wibox = { widget = { textclock = require("wibox.widget.textclock") }, --DOC_HIDE
    layout = { align = require("wibox.layout.align") } } --DOC_HIDE

    local mytextclock = wibox.widget.textclock()

--DOC_NEWLINE

local wb = awful.wibar { position = "top" } --DOC_HIDE

wb:setup { layout = wibox.layout.align.horizontal, --DOC_HIDE
    nil, nil, mytextclock} --DOC_HIDE

require("gears.timer").run_delayed_calls_now() --DOC_HIDE the hierarchy is async

    local myclock_t = awful.tooltip {
        objects        = { mytextclock },
        timer_function = function()
            return os.date("Today is %A %B %d %Y\nThe time is %T")
        end,
    }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE

mouse.coords{x=250, y= 10} --DOC_HIDE
mouse.push_history() --DOC_HIDE

assert(myclock_t.wibox and myclock_t.wibox.visible) --DOC_HIDE
