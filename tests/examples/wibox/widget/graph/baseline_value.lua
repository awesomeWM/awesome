--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

local data = { --DOC_HIDE
    1, 2, 3, 4, 5, 0, -5, -4, -3, -2, -1, --DOC_HIDE
} --DOC_HIDE

local w_normal = --DOC_HIDE
wibox.widget {
    --baseline_value = 0, --default
    min_value      = -5,
    max_value      = 5,
    step_width     = 9, --DOC_HIDE
    step_spacing   = 1, --DOC_HIDE
    border_width   = 2, --DOC_HIDE
    margins        = 5, --DOC_HIDE
    forced_height  = 104, --DOC_HIDE
    widget         = wibox.widget.graph,
}

--DOC_NEWLINE

local w_plus = --DOC_HIDE
wibox.widget {
    baseline_value = 5,
    min_value      = -5,
    max_value      = 5,
    step_width     = 9, --DOC_HIDE
    step_spacing   = 1, --DOC_HIDE
    border_width   = 2, --DOC_HIDE
    margins        = 5, --DOC_HIDE
    forced_height  = 104, --DOC_HIDE
    widget         = wibox.widget.graph,
}

--DOC_NEWLINE

local w_minus = --DOC_HIDE
wibox.widget {
    baseline_value = -2.5,
    min_value      = -5,
    max_value      = 5,
    step_width     = 9, --DOC_HIDE
    step_spacing   = 1, --DOC_HIDE
    border_width   = 2, --DOC_HIDE
    margins        = 5, --DOC_HIDE
    forced_height  = 104, --DOC_HIDE
    widget         = wibox.widget.graph,
}

--DOC_NEWLINE

for _, v in ipairs(data) do --DOC_HIDE
    w_normal:add_value(v) --DOC_HIDE
    w_plus:add_value(v) --DOC_HIDE
    w_minus:add_value(v) --DOC_HIDE
end --DOC_HIDE

parent:add(wibox.layout {--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>baseline_value = 0</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_normal,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>baseline_value = 5</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_plus,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>baseline_value = -2.5</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_minus,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE

    forced_width  = 342, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
}) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
