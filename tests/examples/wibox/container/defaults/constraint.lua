--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local wibox = require("wibox")

return {
    text   = "Some long text",
    align  = "center",
    valign = "center",
    widget = wibox.widget.textbox,
},
{
    {
        {
            {
                text   = "Some long text",
                align  = "center",
                valign = "center",
                widget = wibox.widget.textbox,
            },
            strategy = "max",
            height   = 8,
            width    = 50,
            widget   = wibox.container.constraint
        },
        layout = wibox.layout.fixed.horizontal,
    },
    layout = wibox.layout.fixed.vertical,
}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
