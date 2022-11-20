--DOC_GEN_IMAGE  --DOC_HIDE
local generic_widget = ... --DOC_NO_USAGE --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

    local l = wibox.widget {
        generic_widget("first"),
        generic_widget("second"),
        generic_widget("third"),
        generic_widget("fourth"),
        generic_widget("fifth"),
        forced_num_cols = 2,
        spacing         = 5,
        min_cols_size   = 10,
        min_rows_size   = 10,
        layout          = wibox.layout.grid,
    }

return l, l:fit({dpi=96}, 300, 200) --DOC_HIDE
