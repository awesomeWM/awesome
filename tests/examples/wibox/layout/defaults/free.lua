local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

local w1 = generic_widget() --DOC_HIDE
w1.point  = {75,5} --DOC_HIDE
w1.text   = "first" --DOC_HIDE
w1.forced_width = 50 --DOC_HIDE

local w2 = generic_widget() --DOC_HIDE
w2.text = "second"
w2.point  = function(cwidth, cheight, wwidth, wheight)
    -- Bottom right
    return {cwidth-wwidth, cheight-wheight}
end

return --DOC_HIDE
wibox.widget {
    w1,
    w2,
    generic_widget("third"),
    layout  = wibox.layout.free
}
