--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local wibox     = require("wibox")
local gears     = {shape = require("gears.shape")}
local beautiful = require("beautiful")

return {
    text   = "Before",
    align  = "center",
    valign = "center",
    widget = wibox.widget.textbox,
},
{
    {
        text   = "After",
        align  = "center",
        valign = "center",
        widget = wibox.widget.textbox,
    },
    shape        = gears.shape.circle,
    border_width = 5,
    border_color = "#ff0000",
    bg           = beautiful.bg_highlight,
    widget       = wibox.container.background
}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
