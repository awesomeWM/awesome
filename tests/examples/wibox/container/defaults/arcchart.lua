--DOC_HIDE_ALL
local wibox     = require("wibox")
local beautiful = require("beautiful")

return {
    text   = "Before",
    align  = "center",
    valign = "center",
    widget = wibox.widget.textbox,
},
{
    {
        {
            text   = "After",
            align  = "center",
            valign = "center",
            widget = wibox.widget.textbox,
        },
        colors = {
            beautiful.bg_normal,
            beautiful.bg_highlight,
            beautiful.border_color,
        },
        values = {
            1,
            2,
            3,
        },
        max_value    = 10,
        min_value    = 0,
        rounded_edge = true,
        bg           = "#ff000033",
        border_width = 0.5,
        border_color = "#000000",
        start_angle  = 0,
        widget       = wibox.container.arcchart
    },
    margins = 2,
    colors = {
        beautiful.bg_normal,
        beautiful.bg_highlight,
        beautiful.border_color,
    },
    layout = wibox.container.margin
}
