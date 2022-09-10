--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local wibox = require("wibox")
local awful = { widget = { only_on_screen = require("awful.widget.only_on_screen") } }

return {
    text   = "Before",
    halign = "center",
    valign = "center",
    widget = wibox.widget.textbox,
},
{
    {
        text   = "After",
        halign = "center",
        valign = "center",
        widget = wibox.widget.textbox,
    },
    screen = "primary",
    widget = awful.widget.only_on_screen
}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
