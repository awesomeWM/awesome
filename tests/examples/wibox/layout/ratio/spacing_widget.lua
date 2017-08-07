local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears     = {shape = require("gears.shape")}--DOC_HIDE

local l = wibox.layout.flex.vertical() --DOC_HIDE

-- Use the separator widget directly
local w1 = wibox.widget {
    generic_widget( "first"   , nil, 0), --DOC_HIDE
    generic_widget( "second"  , nil, 0), --DOC_HIDE
    generic_widget( "third"   , nil, 0), --DOC_HIDE
    spacing        = 10,
    spacing_widget = wibox.widget.separator,
    layout         = wibox.layout.ratio.horizontal
}

l:add(w1) --DOC_HIDE

-- Use a standard declarative widget construct
local w2 = wibox.widget {
    generic_widget( "first"  ), --DOC_HIDE
    generic_widget( "second" ), --DOC_HIDE
    generic_widget( "third"  ), --DOC_HIDE
    spacing = 10,
    spacing_widget = {
        color  = "#00ff00",
        shape  = gears.shape.circle,
        widget = wibox.widget.separator,
    },
    layout = wibox.layout.ratio.horizontal
}

l:add(w2) --DOC_HIDE

-- Use composed widgets
local w3 = wibox.widget {
    generic_widget( "first"  ), --DOC_HIDE
    generic_widget( "second" ), --DOC_HIDE
    generic_widget( "third"  ), --DOC_HIDE
    spacing = 10,
    spacing_widget = {
        {
            text   = "F",
            widget = wibox.widget.textbox,
        },
        bg     = "#ff0000",
        widget = wibox.container.background,
    },
    layout = wibox.layout.ratio.horizontal
}

l:add(w3) --DOC_HIDE

-- Use negative spacing to create a powerline effect
local w4 = wibox.widget {
    generic_widget( "    first     "   , nil, 0), --DOC_HIDE
    generic_widget( "     second     " , nil, 0), --DOC_HIDE
    generic_widget( "    third     "   , nil, 0), --DOC_HIDE
    spacing = -12,
    spacing_widget = {
        color  = "#ff0000",
        shape  = gears.shape.powerline,
        widget = wibox.widget.separator,
    },
    layout = wibox.layout.ratio.horizontal
}

l:add(w4) --DOC_HIDE

return l, 250, 4*30 --DOC_HIDE
