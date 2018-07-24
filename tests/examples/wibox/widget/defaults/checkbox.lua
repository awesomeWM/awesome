--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local gears     = {shape = require("gears.shape")} --DOC_HIDE

parent:add( --DOC_HIDE
    wibox.widget { --DOC_HIDE
        wibox.widget {
            checked       = true,
            color         = beautiful.bg_normal,
            paddings      = 2,
            forced_width  = 20, --DOC_HIDE
            forced_height = 20, --DOC_HIDE
            shape         = gears.shape.circle,
            widget        = wibox.widget.checkbox
        }
        , --DOC_HIDE
        wibox.widget { --DOC_HIDE
            checked       = false, --DOC_HIDE
            color         = beautiful.bg_normal, --DOC_HIDE
            paddings      = 2, --DOC_HIDE
            forced_width  = 20, --DOC_HIDE
            forced_height = 20, --DOC_HIDE
            shape         = gears.shape.circle, --DOC_HIDE
            widget        = wibox.widget.checkbox --DOC_HIDE
        }, --DOC_HIDE
        spacing = 4, --DOC_HIDE
        layout = wibox.layout.fixed.horizontal --DOC_HIDE
    } --DOC_HIDE

) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
