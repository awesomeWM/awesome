--- Tests for spawn

local spawn = require("awful.spawn")

local spawns_done = 0

local steps = {
  function(count)
    if count == 1 then
      local steps_yay = 0
      spawn.with_line_callback("echo yay", function(line)
        assert(line == "yay", "line == '" .. tostring(line) .. "'")
        assert(steps_yay == 0)
        steps_yay = steps_yay + 1
      end, nil, function()
        assert(steps_yay == 1)
        steps_yay = steps_yay + 1
        spawns_done = spawns_done + 1
      end)

      local steps_count = 0
      local err_count = 0
      spawn.with_line_callback({ "sh", "-c", "printf line1\\\\nline2\\\\nline3 ; echo err >&2" },
      function(line)
        assert(steps_count < 3)
        steps_count = steps_count + 1
        assert(line == "line" .. steps_count, "line == '" .. tostring(line) .. "'")
      end, function(line)
        assert(err_count == 0)
        err_count = err_count + 1
        assert(line == "err", "line == '" .. tostring(line) .. "'")
      end, function()
        assert(steps_count == 3)
        assert(err_count == 1)
        steps_count = steps_count + 1
        spawns_done = spawns_done + 1
      end)
    end
    if spawns_done == 2 then
      return true
    end
  end,
}

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
