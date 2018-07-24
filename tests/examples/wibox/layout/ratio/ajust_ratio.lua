--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1) --DOC_HIDE

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

local values = {
    {0.25, 0.50, 0.25},
    {0.33, 0.55, 0.12},
    {0.123, 0.456, 0.789},
    {0.123, 0, 0.789},
    {0, 1, 0},
}

for i=1, 5 do
    ret:add(wibox.widget { --DOC_HIDE
        markup = "<b>Set " .. i ..":</b>", --DOC_HIDE
        widget = wibox.widget.textbox --DOC_HIDE
    }) --DOC_HIDE

    local w = create() --DOC_HIDE

    for _=1, i do --DOC_HIDE
    w:ajust_ratio(2, unpack(values[i]))
    end --DOC_HIDE

    ret:add(w) --DOC_HIDE
end

return ret, 200, 200 --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
