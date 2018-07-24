--DOC_GEN_IMAGE
local generic_widget, generic_before_after = ... --DOC_HIDE_ALL
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

print("l:add_widget_at(new, 1, 4, 1, 1)") --DOC_HIDE

local w = generic_before_after(wibox.layout.grid, {
    forced_num_cols = 3,
    forced_num_rows = 2,
    homogeneous     = true,
}, 6, "add_widget_at", {--DOC_HIDE
    generic_widget("__new__",beautiful.bg_highlight) --DOC_HIDE
    , 1, 4, 1, 1 --DOC_HIDE
}) --DOC_HIDE

return w, w:fit({dpi=96}, 9999, 9999) --DOC_HIDE
