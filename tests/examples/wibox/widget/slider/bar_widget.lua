--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent = ...
local wibox     = require("wibox")
local beautiful = require("beautiful")

local function gen(val)
    return wibox.widget {
        bar_border_color    = beautiful.border_color,
        bar_border_width    = 1,
        bar_margins         = {},
        bar_widget        = wibox.widget {
            widget = wibox.widget.textbox,
            text = "Some text."
        },
        handle_color        = beautiful.bg_normal,
        handle_border_color = beautiful.border_color,
        handle_border_width = 1,
        widget              = wibox.widget.slider,
    }
end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
