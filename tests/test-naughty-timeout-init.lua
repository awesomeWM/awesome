-- Test that setting timeout=0 on an existing notification stops and clears
-- the old timer (regression test for set_timeout() only stopping the timer
-- inside the timeout > 0 branch).

require("naughty")
local notification = require("naughty.notification")

local steps = {}

table.insert(steps, function()
    -- Create a notification with a 5-second timeout (starts a timer).
    local n = notification {
        title   = "timeout-test",
        timeout = 5,
    }

    assert(n.timer, "expected a timer after timeout=5")
    assert(n.timer.started, "expected timer to be started")

    -- Now set timeout=0 ("never expire").  The old timer must be stopped
    -- and the reference cleared.
    n.timeout = 0

    assert(not n.timer, "expected timer to be nil after timeout=0")

    -- reset_timeout() should be safe and not restart a stale timer.
    n:reset_timeout()

    assert(not n.timer, "expected timer to still be nil after reset_timeout()")

    -- Clean up.
    n:destroy()

    return true
end)

require("_runner").run_steps(steps)
