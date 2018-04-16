local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

local widget = wibox.widget {
    {
        text   = "Useless example!",
        when   = function() return true end,
        widget = wibox.widget.textbox
    },

    connected_global_signals = {
        {client, "focused"  },
        {tag   , "selected" },
        {tag   , "activated"},
    },

    layout  = wibox.container.conditional
}

parent:add(widget) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
