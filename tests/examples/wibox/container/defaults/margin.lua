--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local wibox     = require("wibox")
local beautiful = require("beautiful")

return {
    nil,
    {
        nil,
        {
            {
                text   = "Before",
                halign = "center",
                valign = "center",
                widget = wibox.widget.textbox,
            },
            bg       = beautiful.bg_highlight,
            widget   = wibox.container.background
        },
        nil,
        expand = "none",
        layout = wibox.layout.align.horizontal,
    },
    nil,
    expand = "none",
    layout = wibox.layout.align.vertical,
},
{
    nil,
    {
        nil,
        {
            {
                {
                    text   = "After",
                    halign = "center",
                    valign = "center",
                    widget = wibox.widget.textbox,
                },
                bg       = beautiful.bg_highlight,
                widget   = wibox.container.background
            },
            top    = 5,
            left   = 20,
            color  = "#ff0000",
            widget = wibox.container.margin,
        },
        nil,
        expand = "none",
        layout = wibox.layout.align.horizontal,
    },
    nil,
    expand = "none",
    layout = wibox.layout.align.vertical,
}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
