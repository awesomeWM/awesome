--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears     = {shape = require("gears.shape")} --DOC_HIDE

local data = { --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
} --DOC_HIDE

local w = --DOC_HIDE
wibox.widget {
    max_value = 29,
    step_width = 3,
    step_spacing = 1,
    step_shape = function(cr, width, height)
        gears.shape.rounded_rect(cr, width, height, 2)
    end,
    widget = wibox.widget.graph
}

parent:add( w ) --DOC_HIDE

for _, v in ipairs(data) do --DOC_HIDE
    w:add_value(v) --DOC_HIDE
end --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
