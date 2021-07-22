--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

--DOC_HIDE for i = 0, 59 do print(math.floor(math.sin(i/30*math.pi)*64 + 0.5)) end
local sine = { --DOC_HIDE
      0,   7,  13,  20,  26,  32,  38,  43,  48,  52,  55,  58,  61,  63,  64, --DOC_HIDE
     64,  64,  63,  61,  58,  55,  52,  48,  43,  38,  32,  26,  20,  13,   7, --DOC_HIDE
      0,  -7, -13, -20, -26, -32, -38, -43, -48, -52, -55, -58, -61, -63, -64, --DOC_HIDE
    -64, -64, -63, -61, -58, -55, -52, -48, -43, -38, -32, -26, -20, -13,  -7, --DOC_HIDE
} --DOC_HIDE

-- The red and blue data groups are constant,
-- but the green data group is a sine,
-- which, when it becomes negative,
-- triggers NaN indication in a stacked graph.

local colors = {
    "#ff0000",
    "#00ff00",
    "#0000ff"
}

--DOC_NEWLINE

local w_default = --DOC_HIDE
wibox.widget {
    --default nan_color
    stack        = true,
    group_colors = colors,
    step_width   = 1,
    step_spacing = 1,
    scale        = true, --DOC_HIDE
    border_width = 2, --DOC_HIDE
    margins      = 5, --DOC_HIDE
    forced_height = 44, --DOC_HIDE
    widget       = wibox.widget.graph,
}

--DOC_NEWLINE

local w_custom = --DOC_HIDE
wibox.widget {
    nan_color    = "#ff00ff7f",
    stack        = true,
    group_colors = colors,
    step_width   = 1,
    step_spacing = 1,
    scale        = true, --DOC_HIDE
    border_width = 2, --DOC_HIDE
    margins      = 5, --DOC_HIDE
    forced_height = 44, --DOC_HIDE
    widget       = wibox.widget.graph,
}

--DOC_NEWLINE

local w_false = --DOC_HIDE
wibox.widget {
    nan_indication = false,
    stack          = true,
    group_colors   = colors,
    step_width     = 1,
    step_spacing   = 1,
    scale          = true, --DOC_HIDE
    border_width   = 2, --DOC_HIDE
    margins        = 5, --DOC_HIDE
    forced_height  = 44, --DOC_HIDE
    widget         = wibox.widget.graph,
}

--DOC_NEWLINE

for _, w in ipairs({w_default, w_custom, w_false}) do --DOC_HIDE
    for idx = 0, #sine-1 do --DOC_HIDE
        w:add_value(32, 1) --DOC_HIDE
        w:add_value(32, 3) --DOC_HIDE
        --DOC_HIDE 64*math.sin(2*idx/30*math.pi)
        local v = sine[((2*idx) % #sine)+1] --DOC_HIDE
        w:add_value(v, 2) --DOC_HIDE
    end --DOC_HIDE
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
