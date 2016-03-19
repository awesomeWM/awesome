local parent    = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local gears     = {shape = require("gears.shape")} --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

parent : setup {
    {
        -- Adding a shape without margin may result in cropped output
        {
            text   = "Hello world!",
            widget = wibox.widget.textbox
        },
        shape              = gears.shape.hexagon,
        bg                 = beautiful.bg_normal,
        shape_border_color = beautiful.border_color,
        shape_border_width = beautiful.border_width,
        widget             = wibox.widget.background
    },
    {
        -- To solve this, use a margin
        {
            {
                text   = "Hello world!",
                widget = wibox.widget.textbox
            },
            left   = 10,
            right  = 10,
            top    = 3,
            bottom = 3,
            widget = wibox.layout.margin
        },
        shape              = gears.shape.hexagon,
        bg                 = beautiful.bg_normal,
        shape_border_color = beautiful.border_color,
        shape_border_width = beautiful.border_width,
        widget             = wibox.widget.background
    },
    spacing = 10,
    layout  = wibox.layout.fixed.vertical
}
