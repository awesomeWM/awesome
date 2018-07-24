--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE

local l = wibox.layout {
    layout  = wibox.layout.manual
}
--
-- Option 1: Set the point directly in the widget
local w1        = generic_widget()
w1.point        = {x=75, y=5}
w1.text         = "first"
w1.forced_width = 50
l:add(w1)

--
-- Option 2: Set the point directly in the widget as a function

local w2  = generic_widget()
w2.text   = "second"
w2.point  = function(geo, args)
    return {
        x = args.parent.width  - geo.width,
        y = 0
    }
end
l:add(w2)

--
-- Option 3: Set the point directly in the widget as an `awful.placement`
-- function.

local w3 = generic_widget()
w3.text  = "third"
w3.point = awful.placement.bottom_right
l:add(w3)

--
-- Option 4: Use `:add_at` instead of using the `.point` property. This works
-- with all 3 ways to define the point.
-- function.

local w4 = generic_widget()
w4.text  = "fourth"
l:add_at(w4, awful.placement.centered + awful.placement.maximize_horizontally)

return l, 200, 100 --DOC_HIDE

