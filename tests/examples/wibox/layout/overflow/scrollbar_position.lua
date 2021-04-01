--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox          = require("wibox") --DOC_HIDE
local gshape = require('gears.shape') --DOC_HIDE

local scrollbar_widget = wibox.widget{ --DOC_HIDE
    widget = wibox.widget.separator, --DOC_HIDE
    shape = gshape.rectangle, --DOC_HIDE
    color = "#00ff00", --DOC_HIDE
} --DOC_HIDE

local w1 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10, --DOC_HIDE
    scrollbar_width = 5, --DOC_HIDE
    scrollbar_position = "bottom",
    scrollbar_widget = scrollbar_widget, --DOC_HIDE
    layout  = wibox.layout.overflow.horizontal
}

local w2 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10, --DOC_HIDE
    scrollbar_width = 5, --DOC_HIDE
    scrollbar_position = "top",
    scrollbar_widget = scrollbar_widget, --DOC_HIDE
    layout  = wibox.layout.overflow.horizontal
}

local w3 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10, --DOC_HIDE
    scrollbar_width = 5, --DOC_HIDE
    scrollbar_position = "left",
    scrollbar_widget = scrollbar_widget, --DOC_HIDE
    layout = wibox.layout.overflow.vertical
}

local w4 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10, --DOC_HIDE
    scrollbar_width = 5, --DOC_HIDE
    scrollbar_position = "right",
    scrollbar_widget = scrollbar_widget, --DOC_HIDE
    layout = wibox.layout.overflow.vertical
}

local ret = wibox.widget{                            --DOC_HIDE
    {                                                --DOC_HIDE
        {                                            --DOC_HIDE
            {                                        --DOC_HIDE
                w1,                                  --DOC_HIDE
                border_width = 1,                    --DOC_HIDE
                widget = wibox.container.background, --DOC_HIDE
            },                                       --DOC_HIDE
            nil,                                     --DOC_HIDE
            {                                        --DOC_HIDE
                w2,                                  --DOC_HIDE
                border_width = 1,                    --DOC_HIDE
                widget = wibox.container.background, --DOC_HIDE
            },                                       --DOC_HIDE
            spacing = 10,                            --DOC_HIDE
            layout = wibox.layout.align.vertical,    --DOC_HIDE
        },                                           --DOC_HIDE
        strategy = "max",                            --DOC_HIDE
        width = 70,                                  --DOC_HIDE
        height = 40,                                 --DOC_HIDE
        widget = wibox.container.constraint,         --DOC_HIDE
    },                                               --DOC_HIDE
    {                                                --DOC_HIDE
        {                                            --DOC_HIDE
            w3,                                      --DOC_HIDE
            border_width = 1,                        --DOC_HIDE
            widget = wibox.container.background,     --DOC_HIDE
        },                                           --DOC_HIDE
        strategy = "max",                            --DOC_HIDE
        width = 40,                                  --DOC_HIDE
        height = 40,                                 --DOC_HIDE
        widget = wibox.container.constraint,         --DOC_HIDE
    },                                               --DOC_HIDE
    {                                                --DOC_HIDE
        {                                            --DOC_HIDE
            w4,                                      --DOC_HIDE
            border_width = 1,                        --DOC_HIDE
            widget = wibox.container.background,     --DOC_HIDE
        },                                           --DOC_HIDE
        strategy = "max",                            --DOC_HIDE
        width = 40,                                  --DOC_HIDE
        height = 40,                                 --DOC_HIDE
        widget = wibox.container.constraint,         --DOC_HIDE
    },                                               --DOC_HIDE
    spacing = 10,                                    --DOC_HIDE
    layout = wibox.layout.fixed.horizontal,          --DOC_HIDE
}                                                    --DOC_HIDE

return ret, 200, 50 --DOC_HIDE
