--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE

parent:add( --DOC_HIDE

wibox.widget {
    data_list = {
        { 'L1', 100 },
        { 'L2', 200 },
        { 'L3', 300 },
    },
    border_width = 1,
    forced_height = 50, --DOC_HIDE
    forced_width  = 100, --DOC_HIDE
    colors = {
        beautiful.bg_normal,
        beautiful.bg_highlight,
        beautiful.border_color,
    },
    widget = wibox.widget.piechart
}

) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
