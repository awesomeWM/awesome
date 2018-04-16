local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE
local gears  = {object = require("gears.object")} --DOC_HIDE

local obj1 = gears.object {
    enable_properties   = true,
    enable_auto_signals = true,
}

local widget = wibox.widget {
    {
        text   = "Foo is not set",
        when   = function() return obj1.foo ~= "bar" end,
        widget = wibox.widget.textbox
    },
    {
        text   = "Foo is set",
        when   = function() return obj1.foo == "bar" end,
        widget = wibox.widget.textbox
    },

    layout  = wibox.container.conditional
}

--DOC_HIDE test that detach_signal works
assert(widget.widget.text == "Foo is not set") --DOC_HIDE
widget:attach_signal(obj1, "property::foo")
widget:detach_signal(obj1, "property::foo") --DOC_HIDE
obj1.foo = "bar" --DOC_HIDE
awesome.emit_signal("refresh") --DOC_HIDE
assert(widget.widget.text == "Foo is not set") --DOC_HIDE
obj1.foo = "baz" --DOC_HIDE

widget:attach_signal(obj1, "property::foo") --DOC_HIDE

obj1.foo = "bar"

--DOC_HIDE verify that the result was updated
awesome.emit_signal("refresh") --DOC_HIDE
assert(widget.widget.text == "Foo is set") --DOC_HIDE

parent:add(widget) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
