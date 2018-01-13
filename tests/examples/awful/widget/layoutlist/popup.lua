local awful = { --DOC_HIDE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {layoutlist = require("awful.widget.layoutlist")} --DOC_HIDE
} --DOC_HIDE
local gears = { shape = require("gears.shape") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

    awful.popup {
        widget = wibox.widget {
            awful.widget.layoutlist {
                screen      = 1,
                base_layout = wibox.widget {
                    spacing         = 5,
                    forced_num_cols = 5,
                    layout          = wibox.layout.grid.vertical(),
                },
                widget_template = {
                    {
                        {
                            id            = 'icon_role',
                            forced_height = 22,
                            forced_width  = 22,
                            widget        = wibox.widget.imagebox,
                        },
                        margins = 4,
                        widget  = wibox.container.margin,
                    },
                    id              = 'background_role',
                    forced_width    = 24,
                    forced_height   = 24,
                    shape           = gears.shape.rounded_rect,
                    widget          = wibox.container.background,
                },
            },
            margins = 4,
            widget  = wibox.container.margin,
        },
        border_color = beautiful.border_color,
        border_width = beautiful.border_width,
        placement    = awful.placement.centered,
        ontop        = true,
        shape        = gears.shape.rounded_rect
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
