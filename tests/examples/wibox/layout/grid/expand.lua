--DOC_GEN_IMAGE
local generic_widget = ... --DOC_HIDE_ALL
local wibox     = require("wibox")
local beautiful = require("beautiful")

local w = wibox.widget {
    {
        {
            {
                markup = "<b>expand</b> = <i>false</i>\n<b>homogeneous</b> = <i>false</i>",
                widget = wibox.widget.textbox
            },
            {
                {
                    generic_widget( "short 1"  ),
                    generic_widget( "-------- long 1 --------" ),
                    generic_widget( "short 2"  ),
                    generic_widget( "-------- long 2 --------" ),
                    forced_num_cols = 2,
                    forced_num_rows = 2,
                    expand          = false,
                    homogeneous     = false,
                    layout          = wibox.layout.grid,
                },
                margins = 1,
                color  = beautiful.border_color,
                layout = wibox.container.margin,
            },
            layout = wibox.layout.fixed.vertical
        },
        {
            {
                markup = "<b>expand</b> = <i>true</i>\n<b>homogeneous</b> = <i>false</i>",
                widget = wibox.widget.textbox
            },
            {
                {
                    generic_widget( "short 1"  ),
                    generic_widget( "-------- long 1 --------" ),
                    generic_widget( "short 2"  ),
                    generic_widget( "-------- long 2 --------" ),
                    forced_num_cols = 2,
                    forced_num_rows = 2,
                    expand          = true,
                    homogeneous     = false,
                    layout          = wibox.layout.grid,
                },
                margins = 1,
                color  = beautiful.border_color,
                layout = wibox.container.margin,
            },
            layout = wibox.layout.fixed.vertical
        },
        layout = wibox.layout.flex.horizontal
    },
    {
        {
            {
                markup = "<b>expand</b> = <i>false</i>\n<b>homogeneous</b> = <i>true</i>",
                widget = wibox.widget.textbox
            },
            {
                {
                    generic_widget( "short 1"  ),
                    generic_widget( "-------- long 1 --------" ),
                    generic_widget( "short 2"  ),
                    generic_widget( "-------- long 2 --------" ),
                    forced_num_cols = 2,
                    forced_num_rows = 2,
                    expand          = false,
                    homogeneous     = true,
                    layout          = wibox.layout.grid,
                },
                margins = 1,
                color  = beautiful.border_color,
                layout = wibox.container.margin,
            },
            layout = wibox.layout.fixed.vertical
        },
        {
            {
                markup = "<b>expand</b> = <i>true</i>\n<b>homogeneous</b> = <i>true</i>",
                widget = wibox.widget.textbox
            },
            {
                {
                    generic_widget( "short 1"  ),
                    generic_widget( "-------- long 1 --------" ),
                    generic_widget( "short 2"  ),
                    generic_widget( "-------- long 2 --------" ),
                    forced_num_cols = 2,
                    forced_num_rows = 2,
                    expand          = true,
                    homogeneous     = true,
                    layout          = wibox.layout.grid,
                },
                margins = 1,
                color  = beautiful.border_color,
                layout = wibox.container.margin,
            },
            layout = wibox.layout.fixed.vertical
        },
        layout = wibox.layout.flex.horizontal
    },
    layout = wibox.layout.fixed.vertical
}
return w, 600, 150
