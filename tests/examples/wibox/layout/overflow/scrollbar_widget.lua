--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears     = {shape = require("gears.shape")}--DOC_HIDE

-- Use the default
local w1 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10,
    layout  = wibox.layout.overflow.horizontal
}

-- Use a standard declarative widget construct
local w2 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10,
    scrollbar_widget = {
        color  = "#00ff00",
        shape  = gears.shape.circle,
        widget = wibox.widget.separator,
    },
    scrollbar_width = 10,
    layout = wibox.layout.overflow.horizontal
}

-- Use composed widgets
local w3 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10,
    scrollbar_widget = {
        {
            text   = "Scrollbar",
            widget = wibox.widget.textbox,
        },
        bg     = "#ff0000",
        widget = wibox.container.background,
    },
    scrollbar_width = 10,
    layout = wibox.layout.overflow.horizontal
}

local ret = wibox.widget{                    --DOC_HIDE
    {                                        --DOC_HIDE
        w1,                                  --DOC_HIDE
        border_width = 1,                    --DOC_HIDE
        widget = wibox.container.background, --DOC_HIDE
    },                                       --DOC_HIDE
    {                                        --DOC_HIDE
        w2,                                  --DOC_HIDE
        border_width = 1,                    --DOC_HIDE
        widget = wibox.container.background, --DOC_HIDE
    },                                       --DOC_HIDE
    {                                        --DOC_HIDE
        w3,                                  --DOC_HIDE
        border_width = 1,                    --DOC_HIDE
        widget = wibox.container.background, --DOC_HIDE
    },                                       --DOC_HIDE
    spacing = 5,                             --DOC_HIDE
    layout = wibox.layout.flex.vertical,     --DOC_HIDE
}                                            --DOC_HIDE

return ret, 100, 130 --DOC_HIDE
