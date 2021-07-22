--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = {shape = require("gears.shape")} --DOC_HIDE

local data = { --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
    {3, 5, 6}, {4, 11,15}, {19,29,17},{17,14,0}, {0,3,1}, {0,0,22}, {17,7,1}, {0,0,5}, --DOC_HIDE
} --DOC_HIDE

local colors = {
    "#ff0000",
    "#00ff00",
    "#0000ff"
}

local thin_arrow_shape = function(cr, width, height)
    gears.shape.arrow(cr, width, height, nil, 1, math.max(0, height-4))
end

--DOC_NEWLINE

local w1 = --DOC_HIDE
wibox.widget {
    --stack         = false, --default
    group_colors  = colors,
    step_width    = 5,
    step_spacing  = 1,
    step_shape    = thin_arrow_shape,
    max_value     = 66, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

local w2 = --DOC_HIDE
wibox.widget {
    stack         = true,
    group_colors  = colors,
    step_width    = 5,
    step_spacing  = 1,
    step_shape    = thin_arrow_shape,
    max_value     = 66, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

for _, v in ipairs(data) do --DOC_HIDE
    for group, value in ipairs(v) do --DOC_HIDE
        w1:add_value(value, group) --DOC_HIDE
        w2:add_value(value, group) --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

parent:add(wibox.layout {--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>stack = false</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w1,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>stack = true</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w2,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE

    forced_width  = 210, --DOC_HIDE
    forced_height = 110, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
}) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
