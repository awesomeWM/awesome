--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = {color = require("gears.color"), shape = require("gears.shape")} --DOC_HIDE

local data = { --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
    3, 5, 6,4, 11,15,19,29,17,17,14,0,0,3,1,0,0, 22, 17,7, 1,0,0,5, --DOC_HIDE
} --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_height = 44, --DOC_HIDE
    forced_width  = 200, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
} --DOC_HIDE

--DOC_NEWLINE

local w1 = --DOC_HIDE
wibox.widget {
    step_width   = 9,
    step_shape   = gears.shape.rounded_bar,
    --group_finish = function(cr)
    --    cr:fill() -- default
    --end,
    max_value    = 29, --DOC_HIDE
    widget       = wibox.widget.graph,
}

--DOC_NEWLINE

local w2 = --DOC_HIDE
wibox.widget {
    step_width   = 9,
    step_shape   = gears.shape.rounded_bar,
    group_start  = 0.5, -- Make vertical lines sharper
    group_finish = function(cr)
        cr:set_line_width(1)
        cr:stroke()
    end,
    max_value    = 29, --DOC_HIDE
    widget       = wibox.widget.graph,
}

--DOC_NEWLINE

local w3 = --DOC_HIDE
wibox.widget {
    step_width   = 9,
    step_shape   = gears.shape.rounded_bar,
    group_start  = 0.5, -- Make vertical lines sharper
    group_finish = function(cr)
        cr:fill_preserve()
        cr:set_source(gears.color("#ffffff"))
        cr:set_line_width(1)
        cr:stroke()
    end,
    max_value    = 29, --DOC_HIDE
    widget       = wibox.widget.graph,
}

for _, v in ipairs(data) do --DOC_HIDE
    w1:add_value(v) --DOC_HIDE
    w2:add_value(v) --DOC_HIDE
    w3:add_value(v) --DOC_HIDE
end --DOC_HIDE

l:add(w1) --DOC_HIDE
l:add(w2) --DOC_HIDE
l:add(w3) --DOC_HIDE
parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
