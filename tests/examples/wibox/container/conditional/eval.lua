local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

local widget = wibox.widget {
    {
        text   = "There is no client",
        when   = function() return client.focus == nil end,
        widget = wibox.widget.textbox
    },
    {
        text   = "There is a client",
        when   = function() return client.focus ~= nil end,
        widget = wibox.widget.textbox
    },

    layout  = wibox.container.conditional
}

client.connect_signal("focus", function(c) -- luacheck: no unused args
    widget:eval()
end)

parent:add(widget) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
