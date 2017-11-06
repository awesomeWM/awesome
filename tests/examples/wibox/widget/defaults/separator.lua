local parent    = ... --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE

parent:add( --DOC_HIDE

wibox.widget {
    forced_height = 20, --DOC_HIDE
    forced_width  = 100, --DOC_HIDE
    widget = wibox.widget.separator
}

) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
