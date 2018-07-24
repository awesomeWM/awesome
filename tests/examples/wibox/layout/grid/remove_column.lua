--DOC_GEN_IMAGE
local _, generic_before_after = ... --DOC_HIDE_ALL
local wibox = require("wibox") --DOC_HIDE

print("l:remove_column(2)") --DOC_HIDE

local w = generic_before_after(wibox.layout.grid, {
    forced_num_cols = 3,
    forced_num_rows = 2,
    homogeneous     = true,
}, 6, "remove_column", {2} --DOC_HIDE
)
return w, w:fit({dpi=96}, 9999, 9999) --DOC_HIDE
