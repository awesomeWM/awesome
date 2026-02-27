-- Test that destroying multiple notifications correctly removes all of them
-- from naughty.active (regression test for missing break in cleanup()).

local naughty = require("naughty")
local notification = require("naughty.notification")

local steps = {}

table.insert(steps, function()
    -- Create 3 persistent notifications.
    for i = 1, 3 do
        notification {
            title   = "test" .. i,
            timeout = 0,
        }
    end

    assert(#naughty.active == 3, "expected 3 active, got " .. #naughty.active)

    return true
end)

table.insert(steps, function()
    -- Destroy them all.  Copy the list first because :destroy() removes
    -- from naughty._active, and ipairs over a shrinking table skips entries.
    local copy = {}
    for _, n in ipairs(naughty.active) do copy[#copy+1] = n end
    for _, n in ipairs(copy) do
        n:destroy()
    end

    assert(#naughty.active == 0, "expected 0 active, got " .. #naughty.active)

    return true
end)

require("_runner").run_steps(steps)
