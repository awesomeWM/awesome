--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

-- Create a class for this object. It will be used as a backup source for
-- methods and accessors. It is also possible to set them directly on the
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

print(o.foo)

o.foo = 42

print(o.foo)

o:method(1, 2, 3)

-- Random properties can also be added, the signal will be emitted automatically.

o:connect_signal("property::something", function(obj, value)
    assert(obj == o)
    print("In the connection handler!", value)
end)

print(o.something)

o.something = "a cow"

print(o.something)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
