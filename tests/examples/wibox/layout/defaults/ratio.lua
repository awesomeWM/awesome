--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

local w = wibox.widget {
    generic_widget( "first"  ),
    generic_widget( "second" ),
    generic_widget( "third"  ),
    layout  = wibox.layout.ratio.horizontal
}

w:ajust_ratio(2, 0.44, 0.33, 0.22)

return w --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
