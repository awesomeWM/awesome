--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = {shape = require("gears.shape") } --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

parent:add( --DOC_HIDE

    wibox.widget {
        value            = 75,
        max_value        = 100,
        border_width     = 2,
        border_color     = beautiful.border_color,
        color            = beautiful.border_color,
        shape            = gears.shape.rounded_bar,
        bar_shape        = gears.shape.rounded_bar,
        clip             = false,
        forced_height    = 30,
        forced_width     = 100,
        paddings         = 5,
        margins          = {
            top    = 12,
            bottom = 12,
        },
        widget = wibox.widget.progressbar,
    }

) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
