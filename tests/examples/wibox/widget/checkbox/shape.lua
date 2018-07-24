--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require( "wibox"     ) --DOC_HIDE
local beautiful = require( "beautiful" ) --DOC_HIDE
local gears     = {shape = require("gears.shape")} --DOC_HIDE

local l = wibox.widget { --DOC_HIDE
    spacing = 4, --DOC_HIDE
    layout  = wibox.layout.fixed.horizontal --DOC_HIDE
} --DOC_HIDE

for _, s in ipairs {"rectangle", "circle", "losange", "octogon"} do

    l:add( --DOC_HIDE
    wibox.widget {
        checked       = true,
        color         = beautiful.bg_normal,
        paddings      = 2,
        forced_width  = 20, --DOC_HIDE
        forced_height = 20, --DOC_HIDE
        shape         = gears.shape[s],
        widget        = wibox.widget.checkbox
    }
    ) --DOC_HIDE

end

parent:add(l) --DOC_HIDE
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
