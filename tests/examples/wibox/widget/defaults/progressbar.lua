local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

parent:add( --DOC_HIDE

wibox.widget {
    max_value = 1,
    value     = 0.33,
    widget    = wibox.widget.progressbar
}

) --DOC_HIDE
