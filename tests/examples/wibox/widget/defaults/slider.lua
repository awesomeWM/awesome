--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local gears     = {shape=require("gears.shape") } --DOC_HIDE

parent:add( --DOC_HIDE

wibox.widget {
    bar_shape           = gears.shape.rounded_rect,
    bar_height          = 3,
    bar_color           = beautiful.border_color,
    handle_color        = beautiful.bg_normal,
    handle_shape        = gears.shape.circle,
    handle_border_color = beautiful.border_color,
    handle_border_width = 1,
    value               = 25,
    widget              = wibox.widget.slider,
    forced_height = 50, --DOC_HIDE
    forced_width  = 100, --DOC_HIDE
}

) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
