--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

local w1, w2 = generic_widget(), generic_widget()
w1.point  = {x=75,y=5}
w1.text   = "first"
w1.forced_width = 50

w2.text = "second"
w2.point  = function(geo, args)
    -- Bottom right
    return {
        x = args.parent.width-geo.width,
        y = args.parent.height-geo.height
    }
end

return --DOC_HIDE
wibox.layout {
    w1,
    w2,
    generic_widget("third"),
    layout  = wibox.layout.manual
}
