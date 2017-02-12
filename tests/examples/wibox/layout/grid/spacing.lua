local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

return --DOC_HIDE
wibox.widget {
    generic_widget( "first"  ),
    generic_widget( "second" ),
    generic_widget( "third"  ),
    generic_widget( "fourth" ),
    generic_widget( "fifth"  ),
    generic_widget( "sixth"  ),
    column_count = 3,
    row_count    = 2,
    fill_space   = false,
    spacing      = 10,
    layout       = wibox.layout.grid
}
, 300, 60 --DOC_HIDE
