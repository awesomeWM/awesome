local naughty = require("naughty")
local notification = require("naughty.notification")

local steps = {}

-- Hijack the naughty.layout.box to track them.
local real_box = getmetatable(naughty.layout.box).__call
local boxes = {}

getmetatable(naughty.layout.box).__call = function(_, args)
    local ret = real_box(_, args)

    table.insert(boxes, ret)

    return ret
end

naughty.suspended = true

local display_count, notifs = 0, setmetatable({}, {__mode = "v"})

naughty.connect_signal("request::display", function()
    display_count = display_count + 1
end)

table.insert(steps, function()
    notifs[1] = notification {
        title = "test1"
    }

    notifs[2] = notification {
        title = "test2"
    }

    assert(display_count == 0)
    assert((not notifs[1].timer) or not notifs[1].timer.started)
    assert((not notifs[2].timer) or not notifs[2].timer.started)

    return true
end)

table.insert(steps, function()
    assert(display_count == 0)

    naughty.suspended = false

    assert(display_count == 2)
    assert(#boxes == 2)
    assert(notifs[1].timer.started)
    assert(notifs[2].timer.started)

    notifs[3] = notification {
        title = "test2"
    }

    naughty.suspended = true

    assert(display_count == 3)

    return true
end)

table.insert(steps, function()
    assert(display_count == 3)
    assert(not boxes[1].visible)
    assert(not boxes[2].visible)
    assert((not notifs[1].timer) or not notifs[1].timer.started)
    assert((not notifs[2].timer) or not notifs[2].timer.started)
    assert((not notifs[3].timer) or not notifs[3].timer.started)

    naughty.suspended = false

    return true
end)

table.insert(steps, function()
    assert(display_count == 6)
    assert(not boxes[1].visible)
    assert(not boxes[2].visible)
    assert(boxes[4].visible)
    assert(boxes[5].visible)
    assert(boxes[6].visible)

    assert(notifs[1].timer.started)
    assert(notifs[2].timer.started)
    assert(notifs[3].timer.started)

    setmetatable(boxes, {__mode = "v"})

    notifs[1]:destroy()
    notifs[2]:destroy()
    notifs[3]:destroy()

    return true
end)

table.insert(steps, function()
    collectgarbage("collect")

    if #notifs > 0 then return end
    if #boxes > 0 then return end

    return true
end)

require("_runner").run_steps(steps)
