local generic_widget = ... --DOC_HIDE_ALL
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local w = wibox.widget {
    {
        {
            markup = "<b>spacing</b> = <i>0</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                generic_widget( "third"  ),
                generic_widget( "fourth" ),
                forced_num_cols = 2,
                forced_num_rows = 2,
                homogeneous     = true,
                spacing         = 0,
                padding         = 0,
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
            markup = "<b>spacing</b> = <i>10</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                generic_widget( "third"  ),
                generic_widget( "fourth" ),
                forced_num_cols = 2,
                forced_num_rows = 2,
                homogeneous     = true,
                spacing         = 10,
                padding         = 0,
                layout          = wibox.layout.grid,
            },
            margins = 1,
            color  = beautiful.border_color,
            layout = wibox.container.margin,
        },
        layout = wibox.layout.fixed.vertical
    },
    layout = wibox.layout.flex.horizontal
}
return w, w:fit({dpi=96}, 9999, 9999)
