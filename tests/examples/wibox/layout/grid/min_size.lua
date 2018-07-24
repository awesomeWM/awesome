--DOC_GEN_IMAGE
local generic_widget = ... --DOC_HIDE_ALL
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local w = wibox.widget {
    {
        {
            markup = "<b>min_cols_size</b> = <i>0</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                forced_num_cols = 2,
                min_cols_size   = 0,
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
            markup = "<b>min_cols_size</b> = <i>100</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                forced_num_cols = 2,
                min_cols_size   = 100,
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
return w, w:fit({dpi=96}, 9999, 9999)
--return w, 500, 50
