local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

return --DOC_HIDE
wibox.widget {
    generic_widget( "first"    ),
    generic_widget( "second"   ),
    generic_widget( "third"    ),
    generic_widget( "fourth"   ),
    generic_widget( "fifth"    ),
    generic_widget( "sixth"    ),
    generic_widget( "seventh"  ),
    generic_widget( "eighth"   ),
    generic_widget( "ninth"    ),
    column_count = 3,
    fill_space   = false,
    border_width = 1,
    border_color = beautiful.border_color,
    layout       = wibox.layout.grid
}
, 300, 80 --DOC_HIDE
