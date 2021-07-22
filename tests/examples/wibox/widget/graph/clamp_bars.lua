--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = { --DOC_HIDE
    matrix = require("gears.matrix"), --DOC_HIDE
    shape = require("gears.shape"), --DOC_HIDE
} --DOC_HIDE

local data = { --DOC_HIDE
    -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, --DOC_HIDE
} --DOC_HIDE

local w_unclamped = --DOC_HIDE
wibox.widget {
    clamp_bars    = false,
    step_width    = 9,
    step_spacing  = 1,
    step_shape    = gears.shape.arrow,
    min_value     = -3, --DOC_HIDE
    max_value     = 3, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    forced_height = 104, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

local w_clamped = --DOC_HIDE
wibox.widget {
    --clamp_bars    = true, --default
    step_width    = 9,
    step_spacing  = 1,
    step_shape    = gears.shape.arrow,
    min_value     = -3, --DOC_HIDE
    max_value     = 3, --DOC_HIDE
    border_width  = 2, --DOC_HIDE
    margins       = 5, --DOC_HIDE
    forced_height = 104, --DOC_HIDE
    widget        = wibox.widget.graph,
}

--DOC_NEWLINE

for _, v in ipairs(data) do --DOC_HIDE
    w_unclamped:add_value(v) --DOC_HIDE
    w_clamped:add_value(v) --DOC_HIDE
end --DOC_HIDE

parent:add(wibox.layout {--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>clamp_bars = false</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_unclamped,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE
    {--DOC_HIDE
        {--DOC_HIDE
            markup = "<b>clamp_bars = true</b>",--DOC_HIDE
            widget = wibox.widget.textbox,--DOC_HIDE
        },--DOC_HIDE
        w_clamped,--DOC_HIDE
        layout = wibox.layout.fixed.vertical,--DOC_HIDE
    },--DOC_HIDE

    forced_width  = 221, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.horizontal --DOC_HIDE
}) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
