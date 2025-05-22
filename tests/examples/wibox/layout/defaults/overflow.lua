--DOC_GEN_IMAGE --DOC_HIDE
local generic_widget = ... --DOC_HIDE
local wibox          = require("wibox") --DOC_HIDE

local first = generic_widget("first widget", nil, 0) --DOC_HIDE
local second = generic_widget("second widget", nil, 0) --DOC_HIDE
local third = generic_widget("third widget", nil, 0) --DOC_HIDE
local fourth = generic_widget("fourth widget", nil, 0) --DOC_HIDE

local w = wibox.widget{
    first,
    second,
    third,
    fourth,
    scrollbar_width = 3,
    spacing = 10,
    layout = wibox.layout.overflow.horizontal,
}

return w, 170 --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
