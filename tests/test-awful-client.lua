local awful = require("awful")

awful.util.deprecate = function() end

local has_spawned = false
local steps = {

function(count)

if count <= 1 and not has_spawned and #client.get() < 2 then
    awful.spawn("xterm")
    awful.spawn("xterm")
    has_spawned = true
elseif #client.get() >= 2 then

-- Test properties
client.focus = client.get()[1]
local c  = client.focus
-- local c2 = client.get()[2]

client.add_signal("property::foo")
c.foo = "bar"

-- Check if the property system works
assert(c.foo == "bar")
assert(c.foo == awful.client.property.get(c, "foo"))

-- Test jumpto

--FIXME doesn't work
-- c2:jump_to()
-- assert(client.focus == c2)
-- awful.client.jumpto(c)
-- assert(client.focus == c)

-- Test moveresize
c.size_hints_honor = false
c:geometry {x=0,y=0,width=50,height=50}
c:relative_move( 100, 100, 50, 50 )
for _,v in pairs(c:geometry()) do
    assert(v == 100)
end
awful.client.moveresize(-25, -25, -25, -25, c )
for _,v in pairs(c:geometry()) do
    assert(v == 75)
end

-- Test movetotag

local t  = tags[mouse.screen][1]
local t2 = tags[mouse.screen][2]

c:tags{t}
assert(c:tags()[1] == t)
c:move_to_tag(t2)
assert(c:tags()[1] == t2)
awful.client.movetotag(t, c)
assert(c:tags()[1] == t)

-- Test toggletag

c:tags{t}
c:toggle_tag(t2)
assert(c:tags()[1] == t2 or c:tags()[2] == t2)
awful.client.toggletag(t2, c)
assert(c:tags()[1] == t and c:tags()[1] ~= t2 and c:tags()[2] == nil)

-- Test movetoscreen
--FIXME I don't have the hardware to test

-- Test floating
assert(c.floating ~= nil and type(c.floating) == "boolean")
c.floating = true
assert(awful.client.floating.get(c))
awful.client.floating.set(c, false)
assert(not c.floating)

return true
end
end
}

require("_runner").run_steps(steps)
