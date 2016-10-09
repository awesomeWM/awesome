local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE

local l = wibox.layout {
    layout  = wibox.layout.manual
}
--
local w1        = generic_widget()
w1.point        = {x=75, y=5}
w1.text         = "first"
w1.forced_width = 50
l:add(w1)

l:move_widget(w1, awful.placement.bottom_right)

return l, 100, 50 --DOC_HIDE

