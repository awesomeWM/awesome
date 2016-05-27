--- Tests for spawn's startup notifications.

local runner = require("_runner")
local test_client = require("_client")

local manage_called, c_snid

client.connect_signal("manage", function(c)
  manage_called = true
  c_snid = c.startup_id
  assert(c.machine == awesome.hostname,
      tostring(c.machine) .. " ~= " .. tostring(awesome.hostname))
end)


local ret, snid
local steps = {
  function(count)
    if count == 1 then
      ret, snid = test_client("foo", "bar", true)
    elseif manage_called then
      assert(ret)
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
      ret, snid = test_client("bar", "foo", false)
    elseif manage_called then
      assert(ret)
      assert(snid == nil)
      assert(c_snid == nil, "c.startup_snid should be nil!")
      return true
    end
  end
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
