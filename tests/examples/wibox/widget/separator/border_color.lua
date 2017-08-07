local parent    = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears     = {shape = require("gears.shape")}--DOC_HIDE
local beautiful = require("beautiful")--DOC_HIDE

parent:add( --DOC_HIDE
wibox.widget {
    shape        = gears.shape.circle,
    color        = "#00000000",
    border_width = 1,
    border_color = beautiful.bg_normal,
    widget       = wibox.widget.separator,
    forced_height = 20, --DOC_HIDE
    forced_width  = 20, --DOC_HIDE
}
) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
