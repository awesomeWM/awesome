-- Test set_{,new}index_miss_handler

local mouse = mouse
local class = tag
local obj = class({})
local handler = require("gears.object.properties")
local wibox = require("wibox")

awesome.connect_signal("debug::index::miss", error)
awesome.connect_signal("debug::newindex::miss", error)

class.set_index_miss_handler(function(o, k)
    assert(o == obj)
    assert(k == "key")
    return 42
end)
assert(obj.key == 42)

local called = false
class.set_newindex_miss_handler(function(o, k, v)
    assert(o == obj)
    assert(k == "key")
    assert(v == 42)
    called = true
end)
obj.key = 42
assert(called)

handler(class, {auto_emit=true})

assert(not obj.key)
obj.key = 1337
assert(obj.key == 1337)

-- The the custom mouse handler
mouse.foo = "bar"
assert(mouse.foo == "bar")

local w = wibox()
w.foo = "bar"
assert(w.foo == "bar")

-- Test if read-only properties really are read-only
screen[1].clients = 42
assert(screen[1].clients ~= 42)

-- Test the wibox declarative widget system (drawin proxy)

local w2 = wibox {
    visible = true,
    wisth = 100,
    height = 100
}

w2:setup{
    {
        text   = "Awesomeness!",
        id     = "main_textbox",
        widget = wibox.widget.textbox,
    },
    id     = "main_background",
    widget = wibox.container.background
}

assert(w2.main_background)
assert(w2:get_children_by_id("main_background")[1])
assert(w2:get_children_by_id("main_textbox")[1])
assert(w2.main_background.main_textbox)
assert(w2.main_background == w2:get_children_by_id("main_background")[1])

require("_runner").run_steps({ function() return true end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
