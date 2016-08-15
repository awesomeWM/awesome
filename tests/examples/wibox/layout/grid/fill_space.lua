local generic_widget = ... --DOC_HIDE_ALL
local wibox     = require("wibox")
local beautiful = require("beautiful")

return --DOC_HIDE
wibox.widget {
    {
        {
            markup = "<b>fill_space</b> = <i>false</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                generic_widget( "third"  ),
                generic_widget( "fourth" ),
                column_count = 2,
                row_count    = 2,
                fill_space   = false,
                layout       = wibox.layout.grid,
            },
            margins = 1,
            color  = beautiful.border_color,
            layout = wibox.container.margin,
        },
        layout = wibox.layout.fixed.vertical
    },
    {
        {
            markup = "<b>fill_space</b> = <i>true</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                generic_widget( "third"  ),
                generic_widget( "fourth" ),
                column_count = 2,
                row_count    = 2,
                fill_space   = true,
                layout       = wibox.layout.grid,
            },
            margins = 1,
            color  = beautiful.border_color,
            layout = wibox.container.margin,
        },
        layout = wibox.layout.fixed.vertical
    },
    layout = wibox.layout.flex.horizontal
}
, 500, 60 --DOC_HIDE
