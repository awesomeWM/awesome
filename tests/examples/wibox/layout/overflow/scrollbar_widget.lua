--DOC_GEN_IMAGE --DOC_HIDE_START
local generic_widget = ...
local wibox     = require("wibox")
local gears     = {shape = require("gears.shape")}
--DOC_HIDE_END

-- Use the default
local w1 = wibox.widget {
    generic_widget( "first"  , nil, 0), --DOC_HIDE
    generic_widget( "second" , nil, 0), --DOC_HIDE
    generic_widget( "third"  , nil, 0), --DOC_HIDE
    spacing = 10,
    layout  = wibox.layout.overflow.horizontal
}

-- Use a simple declarative widget construct
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

-- Use a composed widget
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

--DOC_HIDE_START
local ret = wibox.widget{
    {
        w1,
        border_width = 1,
        widget = wibox.container.background,
    },
    {
        w2,
        border_width = 1,
        widget = wibox.container.background,
    },
    {
        w3,
        border_width = 1,
        widget = wibox.container.background,
    },
    spacing = 5,
    layout = wibox.layout.flex.vertical,
}

return ret, 75, 130
