--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local parent    = ...
local wibox     = require("wibox")
local beautiful = require("beautiful")

local function gen(val)
    return wibox.widget {
        bar_border_color    = val,
        bar_border_width    = 4,
        bar_margins         = {},
        handle_color        = beautiful.bg_normal,
        handle_border_color = beautiful.border_color,
        handle_border_width = 1,
        widget              = wibox.widget.slider,
    }
end

local l = wibox.layout {
    gen("#ff0000"), gen("#00ff00"), gen("#0000ff"), gen("#ff00ff"),
    forced_height = 30,
    forced_width  = 400,
    spacing       = 5,
    layout        = wibox.layout.flex.horizontal
}

parent:add(l)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
