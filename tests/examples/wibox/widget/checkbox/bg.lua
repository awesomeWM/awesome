--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE

parent:add( --DOC_HIDE
wibox.widget {
    checked            = true,
    color              = beautiful.bg_normal,
    bg                 = "#ff00ff",
    border_width       = 3,
    paddings           = 4,
    border_color       = "#0000ff",
    check_color        = "#ff0000",
    forced_width       = 20, --DOC_HIDE
    forced_height      = 20, --DOC_HIDE
    check_border_color = "#ffff00",
    check_border_width = 1,
    widget             = wibox.widget.checkbox
}
) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
