local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

local first, second, third = generic_widget("first"), --DOC_HIDE
    generic_widget("second"), generic_widget("third") --DOC_HIDE

local ret = wibox.layout.fixed.vertical()

local function create() --DOC_HIDE

local w = wibox.widget {
    first,
    second,
    third,
    force_width = 200, --DOC_HIDE
    layout  = wibox.layout.ratio.horizontal
}

    return w --DOC_HIDE
end --DOC_HIDE

for i=1, 5 do
    ret:add(wibox.widget { --DOC_HIDE
        markup = "<b>Iteration " .. i ..":</b>", --DOC_HIDE
        widget = wibox.widget.textbox --DOC_HIDE
    }) --DOC_HIDE

    local w = create() --DOC_HIDE

    for _=1, i do --DOC_HIDE
    w:inc_ratio(2, 0.1)
    end --DOC_HIDE

    ret:add(w) --DOC_HIDE
end

return ret, 200, 200 --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
