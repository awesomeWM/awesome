local _, generic_before_after = ... --DOC_HIDE_ALL
local wibox = require("wibox")

print("l:remove(3)")

local w = generic_before_after(wibox.layout.grid, {
    column_count = 3,
    row_count    = 2,
    fill_space   = false,
    spacing      = 10,
}, 6, "remove", {3})

return w, w:fit({dpi=96}, 9999, 9999)
