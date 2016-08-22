local awful = require("awful")
local beautiful = require("beautiful")

awful.util.deprecate = function() end

local function check_order()
    local tags = mouse.screen.tags

    for k, v in ipairs(tags) do
        assert(k == v.index)
    end
end

local has_spawned = false
local steps = {

function(count)

if count <= 1 and not has_spawned and #client.get() < 2 then
    awful.spawn("xterm")
    awful.spawn("xterm")
    has_spawned = true
elseif #client.get() >= 2 then

-- Test move, swap and index

local tags = mouse.screen.tags

assert(#mouse.screen.tags == 9)

check_order()

tags[7].index = 9
assert(tags[7].index == 9)

check_order()

tags[7].index = 4
assert(tags[7].index == 4)

check_order()

awful.tag.move(5, tags[7])
assert(tags[7].index == 5)

check_order()

tags[1]:swap(tags[3])

check_order()

assert(tags[1].index == 3)
assert(tags[3].index == 1)

check_order()

awful.tag.swap(tags[1], tags[3])

assert(tags[3].index == 3)
assert(tags[1].index == 1)

check_order()

-- Test add, icon and delete

client.focus = client.get()[1]
local c  = client.focus
assert(c and client.focus == c)
assert(beautiful.awesome_icon)

local t = awful.tag.add("Test", {clients={c}, icon = beautiful.awesome_icon})

check_order()

local found = false

tags = mouse.screen.tags

assert(#tags == 10)

for _, v in ipairs(tags) do
    if t == v then
        found = true
        break
    end
end

assert(found)

assert(t:clients()[1] == c)
assert(c:tags()[2] == t)
assert(t.icon == beautiful.awesome_icon)

t:delete()

tags = mouse.screen.tags
assert(#tags == 9)

found = false

for _, v in ipairs(tags) do
    if t == v then
        found = true
        break
    end
end

assert(not found)

-- Test selected tags, view only and selected()

t = tags[2]

assert(not t.selected)

assert(t.screen.selected_tag == tags[1])

t:view_only()

assert(t.selected)

assert(not tags[1].selected)

assert(#t.screen.selected_tags == 1)

return true
end
end
}

require("_runner").run_steps(steps)
