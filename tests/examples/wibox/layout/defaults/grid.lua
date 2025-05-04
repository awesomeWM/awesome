--DOC_GEN_IMAGE --DOC_HIDE
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
    homogeneous  = true,
    expand       = true,
    layout       = wibox.layout.grid
}
, nil, 60 --DOC_HIDE
