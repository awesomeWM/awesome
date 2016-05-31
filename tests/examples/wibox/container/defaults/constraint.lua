--DOC_HIDE_ALL
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
