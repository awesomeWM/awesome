--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

local first, second, third = generic_widget("first"), --DOC_HIDE
    generic_widget("second"), generic_widget("third") --DOC_HIDE

local ret = wibox.layout.fixed.vertical() --DOC_HIDE

for i=1, 5 do
    ret:add(wibox.widget { --DOC_HIDE
        markup = "<b>Iteration " .. i ..":</b>", --DOC_HIDE
        widget = wibox.widget.textbox --DOC_HIDE
    }) --DOC_HIDE

    local w = wibox.widget {
        first,
        second,
        third,
        spacing = i*3,
        layout  = wibox.layout.fixed.horizontal
    }

    ret:add(w) --DOC_HIDE
end

return ret, 200, 200 --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
