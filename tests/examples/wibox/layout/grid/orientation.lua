local generic_widget = ... --DOC_HIDE_ALL
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

print([[l = wibox.layout {
    forced_num_cols = 2,
    forced_num_rows = 2,
    homogeneous     = true,
    layout = wibox.layout.grid
}
l:set_orientation("vertical") -- change to "horizontal"
l:add(...)
]]) --DOC_HIDE

return --DOC_HIDE
wibox.widget {
    {
        {
            markup = "<b>orientation</b> = <i>'vertical'</i>",
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
                forced_num_cols = 2,
                forced_num_rows = 2,
                orientation     = "vertical",
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
                forced_num_cols = 2,
                forced_num_rows = 2,
                orientation     = 'horizontal',
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
    layout = wibox.layout.fixed.horizontal
}
, 300, 90 --DOC_HIDE
