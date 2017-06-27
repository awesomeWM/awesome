local generic_widget = ... --DOC_HIDE_ALL
local wibox     = require("wibox")

local empty_width = wibox.widget {
    visible = false
}
local first, third, fourth = generic_widget("first"), generic_widget("third"), generic_widget("fourth")

local function add(tab, name)
    table.insert(tab, {
        markup = "<b>"..name..":</b>",
        widget = wibox.widget.textbox
    })
    table.insert(tab, {
        first,
        empty_width,
        third,
        fourth,
        inner_fill_strategy = name,
        force_width = 200,
        layout  = wibox.layout.ratio.horizontal
    })
end

local ret = {layout = wibox.layout.fixed.vertical}
add(ret, "default")
add(ret, "center")
add(ret, "justify")
add(ret, "inner_spacing")
add(ret, "spacing")
add(ret, "left")
add(ret, "right")

return wibox.widget(ret), 200, 250

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
