local timer = require("gears.timer")
local awful = require("awful")

runner = {
  quit_awesome_on_error = os.getenv('TEST_PAUSE_ON_ERRORS') ~= '1',
  -- quit-on-timeout defaults to false: indicates some problem with the test itself.
  quit_awesome_on_timeout = os.getenv('TEST_QUIT_ON_TIMEOUT') ~= '1',
}

-- Helpers.

--- Add some rules to awful.rules.rules, after the defaults.
local default_rules = awful.util.table.clone(awful.rules.rules)
runner.add_to_default_rules = function(r)
  awful.rules.rules = awful.util.table.clone(default_rules)
  table.insert(awful.rules.rules, r)
end


runner.run_steps = function(steps)
  -- Setup timer/timeout to limit waiting for signal and quitting awesome.
  -- This would be common for all tests.
  local t = timer({timeout=0.1})
  local wait=20
  local step=1
  local step_count=0
  t:connect_signal("timeout", function() timer.delayed_call(function()
    io.flush()  -- for "tail -f".
    step_count = step_count + 1
    local step_as_string = step..'/'..#steps..' (@'..step_count..')'

    -- Call the current step's function.
    local success, result = xpcall(function()
        return steps[step](step_count)
      end, debug.traceback)

    if not success then
      io.stderr:write('Error: running function for step '
        ..step_as_string..': '..tostring(result)..'!\n')
      t:stop()
      if not runner.quit_awesome_on_error then
        io.stderr:write("Keeping awesome open...\n")
        return  -- keep awesome open on error.
      end

    elseif result then
      -- true: test succeeded.
      if step < #steps then
        -- Next step.
        step = step+1
        step_count = 0
        wait = 5
        t:again()
        return
      end

    elseif result == false then
      io.stderr:write("Step "..step_as_string.." failed (returned false).")
      if not runner.quit_awesome_on_error then
        io.stderr:write("Keeping awesome open...\n")
        return
      end

    else
      wait = wait-1
      if wait > 0 then
        t:again()
        return
      else
        io.stderr:write("Error: timeout waiting for signal in step "
          ..step_as_string..".\n")
        t:stop()
        if not runner.quit_awesome_on_timeout then
          return
        end
      end
    end
    -- Remove any clients.
    for _,c in ipairs(client.get()) do
      c:kill()
    end
    awesome.quit()
  end) end)
  t:start()
end

return runner
