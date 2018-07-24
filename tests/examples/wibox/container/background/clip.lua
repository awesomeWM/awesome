--DOC_GEN_IMAGE --DOC_HIDE
local parent    = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears     = {shape = require("gears.shape")} --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

parent : setup {
    {
        -- Some content may be outside of the shape
        {
            text   = "Hello\nworld!",
            widget = wibox.widget.textbox
        },
        shape              = gears.shape.circle,
        bg                 = beautiful.bg_normal,
        shape_border_color = beautiful.border_color,
        widget             = wibox.container.background
    },
    {
        -- To solve this, clip the content
        {
            text   = "Hello\nworld!",
            widget = wibox.widget.textbox
        },
        shape_clip         = true,
        shape              = gears.shape.circle,
        bg                 = beautiful.bg_normal,
        shape_border_color = beautiful.border_color,
        widget             = wibox.container.background
    },
    spacing = 10,
    layout  = wibox.layout.fixed.vertical
}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
