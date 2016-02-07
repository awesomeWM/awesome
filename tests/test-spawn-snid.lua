--- Tests for spawn's startup notifications.

local spawn = require("awful.spawn")

local manage_called, c_snid

client.connect_signal("manage", function(c)
  manage_called = true
  c_snid = c.startup_id
end)


local ret, snid
local steps = {
  function(count)
    if count == 1 then
      ret, snid = spawn('urxvt', true)
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
      ret, snid = spawn('urxvt', false)
    elseif manage_called then
      assert(ret)
      assert(snid == nil)
      assert(c_snid == nil, "c.startup_snid should be nil!")
      return true
    end
  end
}

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
