--DOC_GEN_IMAGE
local generic_widget = ... --DOC_HIDE_START
local wibox     = require("wibox")
local beautiful = require("beautiful")

return
wibox.widget {
    {
        {
            markup = "<b>orientation</b> = <i>'vertical'</i>",
            widget = wibox.widget.textbox
        },
        { --DOC_HIDE_STOP
            {--DOC_HIDE_START
                generic_widget( "first"  ),
                generic_widget( "second" ),
                generic_widget( "third"  ),
                generic_widget( "fourth" ),
                generic_widget( "fifth"  ),
                generic_widget( "sixth"  ),--DOC_HIDE_STOP
                column_count = 2,
                row_count    = 2,
                orientation  = "vertical",
                expand       = false,
                homogeneous  = true,
                layout       = wibox.layout.grid,
            },
            margins = 1,--DOC_HIDE_START
            color   = beautiful.border_color,
            layout  = wibox.container.margin,
        },
        layout = wibox.layout.fixed.vertical
    },
    {
        {
            markup = "<b>orientation</b> = <i>'horizontal'</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                generic_widget( "third"  ),
                generic_widget( "fourth" ),
                generic_widget( "fifth"  ),
                generic_widget( "sixth"  ),
                column_count = 2,
                row_count    = 2,
                orientation  = 'horizontal',
                expand       = false,
                homogeneous  = true,
                layout       = wibox.layout.grid,
            },
            margins = 1,
            color   = beautiful.border_color,
            layout  = wibox.container.margin,
        },
        layout = wibox.layout.fixed.vertical
    },
    spacing = 5,
    layout = wibox.layout.fixed.horizontal
}
, 300, 90
