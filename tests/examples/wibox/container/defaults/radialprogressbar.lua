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
        value              = 0.5,
        max_value          = 1,
        min_value          = 0,
        widget             = wibox.container.radialprogressbar
    },
    margins = 5,
    layout = wibox.container.margin
}
