local runner = require("_runner")
local naughty = require("naughty")

local steps = {}

local n

-- Regression test for #3625:
-- Make sure that setting `timeout = 0` on an existing notification
-- correctly stops and removes the timer.
table.insert(steps, function()
    n = naughty.notification({ text = "regression #3625", timeout = 1 })
    n.timeout = 0
    return true
end)

-- Wait for the notification's timeout to pass.
table.insert(steps, function()
    os.execute("sleep 2")
    return true
end)

table.insert(steps, function()
    assert(not n.is_expired)
    return true
end)

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
