--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

local w1 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10,
    scrollbar_width = 5,
    layout  = wibox.layout.overflow.horizontal
}

local w2 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10,
    scrollbar_width = 10,
    layout = wibox.layout.overflow.horizontal
}

local w3 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10,
    scrollbar_width = 20,
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

return ret, 100, 3*40 --DOC_HIDE
