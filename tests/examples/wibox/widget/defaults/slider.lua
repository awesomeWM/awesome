--DOC_GEN_IMAGE --DOC_HIDE_START
local parent    = ...
local wibox     = require( "wibox"     )
local beautiful = require( "beautiful" )
local gears     = {shape=require("gears.shape") }
local naughty = { notify = function(_) end }

--DOC_HIDE_END
local widget = wibox.widget {
    bar_shape           = gears.shape.rounded_rect,
    bar_height          = 3,
    bar_color           = beautiful.border_color,
    handle_color        = beautiful.bg_normal,
    handle_shape        = gears.shape.circle,
    handle_border_color = beautiful.border_color,
    handle_border_width = 1,
    value               = 25,
    widget              = wibox.widget.slider,
    forced_height = 50, --DOC_HIDE
    forced_width  = 100, --DOC_HIDE
}
--DOC_NEWLINE
-- Connect to `property::value` to use the value on change
widget:connect_signal("property::value", function(_, new_value)
    --DOC_HIDE_START
    if new_value ~= 10 then
        error(string.format("unexpected value %s", new_value))
    end
    --DOC_HIDE_END
    naughty.notify { title = "Slider changed", message = tostring(new_value) }
end)

--DOC_HIDE_START
widget.value = 10
parent:add(widget)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
