--DOC_GEN_IMAGE
local generic_widget_ = ... --DOC_HIDE_ALL
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local function generic_widget(txt)
    return generic_widget_(txt, nil, 0)
end

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
                column_count = 2,
                row_count    = 2,
                homogeneous  = true,
                spacing      = 0,
                layout       = wibox.layout.grid,
            },
            margins = 1,
            color   = beautiful.border_color,
            layout  = wibox.container.margin,
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
                column_count = 2,
                row_count    = 2,
                homogeneous  = true,
                spacing      = 10,
                layout       = wibox.layout.grid,
            },
            margins = 1,
            color   = beautiful.border_color,
            layout  = wibox.container.margin,
        },
        layout = wibox.layout.fixed.vertical
    },
    layout = wibox.layout.flex.horizontal
}
return w, w:fit({dpi=96}, 9999, 9999)
