--DOC_GEN_IMAGE
local generic_widget = ... --DOC_HIDE_ALL
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local w = wibox.widget {
    {
        {
            markup = "<b>minimum_column_width</b> = <i>0</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                column_count         = 2,
                minimum_column_width = 0,
                homogeneous          = true,
                layout               = wibox.layout.grid,
            },
            margins = 1,
            color   = beautiful.border_color,
            layout  = wibox.container.margin,
        },
        layout = wibox.layout.fixed.vertical
    },
    {
        {
            markup = "<b>minimum_column_width</b> = <i>100</i>",
            widget = wibox.widget.textbox
        },
        {
            {
                generic_widget( "first"  ),
                generic_widget( "second" ),
                column_count         = 2,
                minimum_column_width = 100,
                homogeneous          = true,
                layout               = wibox.layout.grid,
            },
            margins = 1,
            color   = beautiful.border_color,
            layout  = wibox.container.margin,
        },
        layout = wibox.layout.fixed.vertical
    },
    layout = wibox.layout.fixed.horizontal
}
return w, w:fit({dpi=96}, 9999, 9999)
--return w, 500, 50
