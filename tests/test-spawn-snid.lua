--- Tests for spawn's startup notifications.

local runner = require("_runner")
local test_client = require("_client")

local manage_called, c_snid

client.connect_signal("request::manage", function(c)
    manage_called = true
    c_snid = c.startup_id
    assert(c.machine == awesome.hostname,
           tostring(c.machine) .. " ~= " .. tostring(awesome.hostname))
end)

local snid
local num_callbacks = 0
local function callback(c)
    assert(c.startup_id == snid)
    num_callbacks = num_callbacks + 1
end

local steps = {
    function(count)
        if count == 1 then
            snid = test_client("foo", "bar", true)
        elseif manage_called then
            assert(snid)
            assert(snid == c_snid)
            return true
        end
    end,

    -- Test that c.startup_id is nil for a client without startup notifications,
    -- and especially not the one from the previous spawn.
    function(count)
        if count == 1 then
            manage_called = false
            test_client("bar", "foo", false)
        elseif manage_called then
            assert(c_snid == nil, "c.startup_snid should be nil!")
            return true
        end
    end,

    function(count)
        if count == 1 then
            manage_called = false
            snid = test_client("baz", "barz", false, callback)
        elseif manage_called then
            assert(snid)
            assert(snid == c_snid)
            assert(num_callbacks == 1, num_callbacks)
            return true
        end
    end,
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
