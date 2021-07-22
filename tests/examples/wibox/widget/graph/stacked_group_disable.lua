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

local colors_normal = {
    "#ff0000",
    "#00ff00", -- the data group is green
    "#0000ff"
}

local colors_transparent = {
    "#ff0000",
    "#00000000", -- the data group is transparent
    "#0000ff"
}

local colors_disabled = {
    "#ff0000",
    nil, -- the data group is disabled
    "#0000ff"
}

--DOC_NEWLINE

local w1 = --DOC_HIDE
wibox.widget {
    stack         = true,
    group_colors  = colors_normal,
    max_value     = 66, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    step_width    = 5, --DOC_HIDE
    step_spacing  = 1, --DOC_HIDE
    step_shape    = gears.shape.rounded_bar, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

local w2 = --DOC_HIDE
wibox.widget {
    stack         = true,
    group_colors  = colors_transparent,
    max_value     = 66, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    step_width    = 5, --DOC_HIDE
    step_spacing  = 1, --DOC_HIDE
    step_shape    = gears.shape.rounded_bar, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

local w3 = --DOC_HIDE
wibox.widget {
    stack         = true,
    group_colors  = colors_disabled,
    max_value     = 66, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    step_width    = 5, --DOC_HIDE
    step_spacing  = 1, --DOC_HIDE
    step_shape    = gears.shape.rounded_bar, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

for _, v in ipairs(data) do --DOC_HIDE
    for group, value in ipairs(v) do --DOC_HIDE
        w1:add_value(value, group) --DOC_HIDE
        w2:add_value(value, group) --DOC_HIDE
        w3:add_value(value, group) --DOC_HIDE
    end --DOC_HIDE
end --DOC_HIDE

parent:add(wibox.layout {--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>Normal</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w1,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>Transparent</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w2,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>Disabled</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w3,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE

    forced_width  = 210, --DOC_HIDE
    forced_height = 110, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
}) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
