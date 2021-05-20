--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

--DOC_HIDE These are not useful examples, but rather a gathering of fringe cases
--DOC_HIDE that should provide code coverage to ensure that we're handling them.

local data = { --DOC_HIDE
    -5, -4, 0/0, -2, -4, 0/0, 4, 2, 3, 0/0, 5, --DOC_HIDE
} --DOC_HIDE

--DOC_HIDE Tests behavior of autoscaling when graph contains only equal values.
--DOC_HIDE This widget will also be rendered multiple times with different
--DOC_HIDE sizes, to test last_drawn_values_num stats gathering.
local w_autoscale_constant = --DOC_HIDE
wibox.widget {
    step_width    = 9,
    step_spacing  = 1,
    scale         = true,
    border_width  = 2, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    forced_height = 44, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

--DOC_HIDE Tests behavior of drawn_values_num calculation, when
--DOC_HIDE border_width > width/2, i.e. there's no free space for drawing.
local w_too_fat_border = --DOC_HIDE
wibox.widget {
    border_width  = 200,
    step_width    = 9,
    step_spacing  = 1,
    scale         = true, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    forced_height = 44, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

--DOC_HIDE Tests behavior of a widget with set capacity property
local w_set_capacity = --DOC_HIDE
wibox.widget {
    capacity      = 5,
    step_width    = 9,
    step_spacing  = 1,
    scale         = true, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    forced_height = 44, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

for _, v in ipairs(data) do --DOC_HIDE
    w_autoscale_constant:add_value(3) --DOC_HIDE adding the same value
    w_too_fat_border:add_value(v) --DOC_HIDE
    w_set_capacity:add_value(v) --DOC_HIDE
end --DOC_HIDE

parent:add(wibox.layout {--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>widget clones</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        {--DOC_HIDE
            w_autoscale_constant,--DOC_HIDE
            w_autoscale_constant,--DOC_HIDE
            forced_height = 22,--DOC_HIDE
            layout = wibox.layout.flex.horizontal,--DOC_HIDE
        },--DOC_HIDE
        {--DOC_HIDE
            w_autoscale_constant,--DOC_HIDE
            w_autoscale_constant,--DOC_HIDE
            w_autoscale_constant,--DOC_HIDE
            forced_height = 22,--DOC_HIDE
            layout = wibox.layout.flex.horizontal,--DOC_HIDE
        },--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>autoscale constant</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_autoscale_constant,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>too fat border</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_too_fat_border,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>set capacity=5</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_set_capacity,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE

    forced_width  = 104, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
}) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
