local gears = require("gears") --DOC_HIDE

 -- Create a class for this object. It will be used as a backup source for
 -- methods and acessors. It is also possible to set them diretly on the
 -- object.
local class = {}
 
function class:get_foo()
    print("In get foo", self._foo or "bar")
    return self._foo or "bar"
end
 
function class:set_foo(value)
    print("In set foo", value)

    -- In case it is necessary to bypass the object property system, use
    -- `rawset`
    rawset(self, "_foo", value)

    -- When using custom accessors, the signals need to be handled manually
    self:emit_signal("property::foo", value)
end

function class:method(a, b, c)
    print("In a mathod", a, b, c)
end

local o = gears.object {
    class               = class,
    enable_properties   = true,
    enable_auto_signals = true,
}

o:add_signal "property::foo"

print(o.foo)
 
o.foo = 42
 
print(o.foo)
 
o:method(1, 2, 3)

 -- Random properties can also be added, the signal will be emited automatically.
o:add_signal "property::something"

o:connect_signal("property::something", function(obj, value)
    print("In the connection handler!", obj, value)
end)

print(o.something)
 
o.something = "a cow"
 
print(o.something)
