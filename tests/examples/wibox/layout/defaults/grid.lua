local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

return --DOC_HIDE
wibox.widget {
    generic_widget( "first"  ),
    generic_widget( "second" ),
    generic_widget( "third"  ),
    generic_widget( "fourth" ),
    column_count = 2,
    row_count    = 2,
    fill_space   = true,
    layout  = wibox.layout.grid
}
, nil, 45 --DOC_HIDE
