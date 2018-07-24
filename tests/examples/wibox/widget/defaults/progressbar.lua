--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = {shape = require("gears.shape") } --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

parent:add( --DOC_HIDE

wibox.widget {
    max_value     = 1,
    value         = 0.33,
    forced_height = 20,
    forced_width  = 100,
    shape         = gears.shape.rounded_bar,
    border_width  = 2,
    border_color  = beautiful.border_color,
    widget        = wibox.widget.progressbar,
}

) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
