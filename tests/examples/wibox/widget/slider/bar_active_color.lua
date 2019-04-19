--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require("wibox")
local beautiful = require("beautiful")

local function gen(val)
    return wibox.widget {
        bar_border_color    = beautiful.border_color,
        bar_border_width    = 1,
        bar_margins         = {},
        bar_color           = "#ffffff",
        bar_active_color    = val,
        handle_border_color = beautiful.border_color,
        handle_border_width = 1,
        handle_color        = beautiful.bg_normal,
        widget              = wibox.widget.slider,
        value               = 40,
        maximum             = 100
    }
end

local l = wibox.layout {
    gen("#bf616a"), gen("#a3be8c"), gen("#5e81ac"), gen("#b48ead"),
    forced_height = 30,
    forced_width  = 400,
    spacing       = 5,
    layout        = wibox.layout.flex.horizontal
}

parent:add(l)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
