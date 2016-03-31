-- Test set_{,new}index_miss_handler

local class = tag
local obj = class({})
local handler = require("gears.object.properties")

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

require("_runner").run_steps({ function() return true end })
