--DOC_HIDE_START
--DOC_GEN_IMAGE
local wibox = require("wibox")

return {
    text   = "Before",
    align  = "center",
    valign = "center",
    widget = wibox.widget.textbox,
},
{
--DOC_HIDE_END
    {
        {
            text   = "After",
            align  = "center",
            valign = "center",
            widget = wibox.widget.textbox,
        },
        value     = 0.5,
        max_value = 1,
        min_value = 0,
        widget    = wibox.container.radialprogressbar
    },
--DOC_HIDE_START
    margins = 5,
    layout = wibox.container.margin
}

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
