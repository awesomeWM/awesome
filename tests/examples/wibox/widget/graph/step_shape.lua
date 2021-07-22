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


local l = wibox.layout { --DOC_HIDE
    forced_height = 200, --DOC_HIDE
    forced_width  = 200, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE

--DOC_NEWLINE

local shapes = {
    gears.shape.squircle,
    gears.shape.octogon,
    gears.shape.rounded_bar,
    gears.shape.arrow
}

--DOC_NEWLINE

for _, shape in ipairs(shapes) do
    local w = --DOC_HIDE

    wibox.widget {
        max_value = 29, --DOC_HIDE
        step_width   = 5,
        step_spacing = 1,
        step_shape   = shape,
        widget       = wibox.widget.graph,
    }

    l:add(w) --DOC_HIDE

    for _, v in ipairs(data) do --DOC_HIDE
        w:add_value(v) --DOC_HIDE
    end --DOC_HIDE

 end

parent:add(l) --DOC_HIDE


--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
