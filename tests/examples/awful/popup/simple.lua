--DOC_GEN_IMAGE --DOC_HIDE
local awful = { placement = require("awful.placement"), --DOC_HIDE
    popup = require("awful.popup") } --DOC_HIDE --DOC_NO_USAGE
local gears = { shape = require("gears.shape") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

    awful.popup {
        widget = {
            {
                {
                    text   = "foobar",
                    widget = wibox.widget.textbox
                },
                {
                    {
                        text   = "foobar",
                        widget = wibox.widget.textbox
                    },
                    bg     = "#ff00ff",
                    clip   = true,
                    shape  = gears.shape.rounded_bar,
                    widget = wibox.widget.background
                },
                {
                    value         = 0.5,
                    forced_height = 30,
                    forced_width  = 100,
                    widget        = wibox.widget.progressbar
                },
                layout = wibox.layout.fixed.vertical,
            },
            margins = 10,
            widget  = wibox.container.margin
        },
        border_color = "#00ff00",
        border_width = 5,
        placement    = awful.placement.top_left,
        shape        = gears.shape.rounded_rect,
        visible      = true,
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
