--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

return --DOC_HIDE
wibox.widget {
    generic_widget( "first"  ),
    generic_widget( "second" ),
    generic_widget( "third"  ),
    generic_widget( "fourth" ),
    forced_num_cols = 2,
    forced_num_rows = 2,
    homogeneous     = true,
    expand          = true,
    layout = wibox.layout.grid
}
, nil, 60 --DOC_HIDE
