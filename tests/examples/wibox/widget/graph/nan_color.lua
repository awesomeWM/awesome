--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = {shape = require("gears.shape")} --DOC_HIDE

local data = {
    -5, -4, 0/0, -2, -1, 0/0, 1, 2, 3, 0/0, 5,
}

--DOC_NEWLINE

local w_default = --DOC_HIDE
wibox.widget {
    -- nan_indication = true, -- default
    -- default nan_color
    step_width    = 9,
    step_spacing  = 1,
    step_shape    = gears.shape.rectangle, --DOC_HIDE
    scale         = true, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    forced_height = 44, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

local w_custom = --DOC_HIDE
wibox.widget {
    -- nan_indication = true, -- default
    nan_color     = "#ff00007f",
    step_width    = 9,
    step_spacing  = 1,
    step_shape    = gears.shape.rectangle, --DOC_HIDE
    scale         = true, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    forced_height = 44, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

local w_false = --DOC_HIDE
wibox.widget {
    nan_indication = false,
    step_width     = 9,
    step_spacing   = 1,
    step_shape     = gears.shape.rectangle, --DOC_HIDE
    scale          = true, --DOC_HIDE
    border_width   = 2, --DOC_HIDE
    margins        = 5, --DOC_HIDE
    forced_height  = 44, --DOC_HIDE
    widget         = wibox.widget.graph,
}

--DOC_NEWLINE

--DOC_HIDE add data in reverse so that visible order corresponds to array order
for i = #data,1,-1 do --DOC_HIDE
    local v = data[i] --DOC_HIDE
    w_default:add_value(v) --DOC_HIDE
    w_custom:add_value(v) --DOC_HIDE
    w_false:add_value(v) --DOC_HIDE
end --DOC_HIDE

parent:add(wibox.layout {--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>default nan_color</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_default,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>custom nan_color</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_custom,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>nan_indication = false</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_false,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE

    forced_width  = 342, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
}) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
