--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears     = {shape = require("gears.shape")}--DOC_HIDE

local wdgs = { --DOC_HIDE
    forced_height = 30, --DOC_HIDE
    forced_width  = 30*4, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
} --DOC_HIDE


for _, s in ipairs { "losange" ,"circle", "isosceles_triangle", "cross" } do
    local w = wibox.widget {
        shape  = gears.shape[s],
        widget = wibox.widget.separator,
    }
    table.insert(wdgs, w) --DOC_HIDE
end

parent:add( wibox.layout(wdgs)) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
