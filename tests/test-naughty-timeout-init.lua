-- Test: Notification timeout bugs.
--
-- 1. Setting timeout=0 doesn't stop existing timer:
--    set_timeout() at notification.lua only stops the old timer inside
--    the `if timeout > 0` block. When timeout is 0 (meaning "never expire"),
--    the else branch never stops the old timer. So setting n.timeout = 0
--    after creation leaves the old timer running.
--
-- 2. Default timeout never starts when no explicit timeout is given:
--    The constructor guard `if n._private.timeout then` skips set_timeout()
--    when timeout is nil (not provided). Without ruled.notification, this
--    means notifications with no explicit timeout never expire.

local naughty = require("naughty")
local notification = require("naughty.notification")
local runner = require("_runner")

-- Register a request::preset handler to enable the new API path.
naughty.connect_signal("request::preset", function(n, context, args)
end)

-- Register a display handler so notifications are "shown"
naughty.connect_signal("request::display", function(n)
    require("naughty.layout.box") { notification = n }
end)

local n_timeout_zero = nil
local n_default = nil

local steps = {}

-- Setting timeout=0 after creation should stop the existing timer.
-- Create notification with timeout=5 (creates a timer), then set timeout=0.
-- The old timer should stop, but it doesn't because the timer-stop code is
-- inside the `if timeout > 0` block.
table.insert(steps, function()
    n_timeout_zero = notification {
        title   = "timeout zero after creation",
        text    = "Setting timeout=0 should stop the timer",
        timeout = 5,
    }

    assert(n_timeout_zero, "notification was not created")
    assert(n_timeout_zero.timer ~= nil, "timer should exist for timeout=5")
    assert(n_timeout_zero.timer.started,
        "timer should be running for timeout=5")

    -- Now set timeout to 0, meaning "never expire"
    n_timeout_zero.timeout = 0

    -- The timeout property should be 0
    assert(n_timeout_zero._private.timeout == 0,
        string.format("expected _private.timeout=0, got %s",
            tostring(n_timeout_zero._private.timeout)))

    -- The old timer (5s) should have been stopped and cleared.
    -- set_timeout(0) must stop the old timer and nil out self.timer so that
    -- reset_timeout() doesn't accidentally restart a stale reference.
    assert(n_timeout_zero.timer == nil,
        "timer should be nil after setting timeout=0")

    return true
end)

-- After setting timeout=0, reset_timeout() must not restart a stale timer.
-- reset_timeout() checks `self.timer and not self.timer.started` and would
-- restart the old (stopped) timer if set_timeout(0) forgot to nil it out.
table.insert(steps, function()
    assert(n_timeout_zero, "notification should still exist")

    n_timeout_zero:reset_timeout()

    assert(n_timeout_zero.timer == nil,
        "timer should be nil after timeout=0; reset_timeout() must not "..
        "restart a stale timer reference")

    return true
end)

-- Default timeout never starts when no explicit timeout given.
table.insert(steps, function()
    n_default = notification {
        title = "default timeout test",
        text  = "Should expire with default timeout",
        -- No timeout specified - should get default from cst.config
    }

    assert(n_default, "notification was not created")

    -- The default timeout from cst.config.defaults is 5 seconds.
    -- set_timeout() should have been called with that value.
    assert(n_default.timer ~= nil,
        "no timer created for notification without explicit timeout - "..
        "constructor guard `if n._private.timeout` skips set_timeout() "..
        "when timeout is nil")

    assert(n_default.timer.started,
        "timer exists but is not started")

    assert(n_default._private.timeout == 5,
        string.format("expected default timeout=5, got %s",
            tostring(n_default._private.timeout)))

    return true
end)

-- Cleanup
table.insert(steps, function()
    if n_timeout_zero and not n_timeout_zero._private.is_destroyed then
        n_timeout_zero:destroy()
    end
    if n_default and not n_default._private.is_destroyed then
        n_default:destroy()
    end
    return true
end)

runner.run_steps(steps, { kill_clients = false })
