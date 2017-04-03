local gears = require("gears") --DOC_HIDE

local o = gears.object{}

-- Add a __tostring metamethod for prettier output
getmetatable(o).__tostring = function()
    return "[obj]"
end

-- Function can be attached to signals
local function slot(obj, a, b, c)
    print("In slot", obj, a, b, c)
end

o:connect_signal("my_signal", slot)

-- Emitting can be done without arguments. In that case, the object will be
-- implicitly added as an argument.
o:emit_signal "my_signal"

-- It is also possible to add as many random arguments are required.
o:emit_signal("my_signal", "foo", "bar", 42)

-- Finally, to allow the object to be garbage collected (the memory freed), it
-- is necessary to disconnect the signal or use `weak_connect_signal`
o:disconnect_signal("my_signal", slot)

-- This time, the `slot` wont be called as it is no longer connected.
o:emit_signal "my_signal"

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
