--DOC_HIDE_ALL
local wibox     = require("wibox")

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
        valign = "bottom",
        halign = "right",
        widget = wibox.container.place
    },
    margins = 5,
    layout = wibox.container.margin
}
