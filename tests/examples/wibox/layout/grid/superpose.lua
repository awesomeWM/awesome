local generic_widget = ... --DOC_HIDE_ALL
local wibox = require("wibox")
local beautiful = require("beautiful")

local w = wibox.widget {
    generic_widget( "first"  ),
    generic_widget( "second" ),
    generic_widget( "third"  ),
    generic_widget( "fourth" ),
    forced_num_cols = 2,
    forced_num_rows = 2,
    superpose       = true,
    homogeneous     = true,
    layout          = wibox.layout.grid,
}
w:add_widget_at(
    generic_widget("fifth",beautiful.bg_highlight)
    , 1, 1, 1, 2)

return w, w:fit({dpi=96}, 9999, 9999)
