local generic_widget, generic_before_after = ... --DOC_HIDE_ALL
local wibox = require("wibox")
local beautiful = require("beautiful")

local w = generic_before_after(wibox.layout.grid, {
    column_count = 3,
    row_count    = 2,
    fill_space   = false,
    spacing      = 10,
}, 6, "set_cell_widget", {2, 1, wibox.widget(
    generic_widget("__new__",beautiful.bg_highlight)
)})

return w, w:fit({dpi=96}, 9999, 9999)
