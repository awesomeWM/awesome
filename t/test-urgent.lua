--- Tests for urgent property.

awful = require("awful")
timer = require("gears.timer")

-- Some basic assertion that the tag is not marked "urgent" already.
assert(awful.tag.getproperty(tags[1][2], "urgent") == nil)


-- Setup signal handler which should be called.
local urgent_cb_done
client.connect_signal("property::urgent", function (c)
  urgent_cb_done = true
  assert(c.class == "XTerm", "Client should be xterm!")
end)


-- Steps to do for this test.
local steps = {
  -- Step 1: tag 2 should become urgent, when a client gets tagged via rules.
  function(count)
    if count == 1 then  -- Setup.
      urgent_cb_done = false
      -- Select first tag.
      awful.tag.viewonly(tags[1][1])

      awful.rules.rules = {
        { rule = { class = "XTerm" },
        properties = { tag = tags[1][2], focus = true } },
      }
      awful.util.spawn("xterm")
    end
    if urgent_cb_done then
      assert(awful.tag.getproperty(tags[1][2], "urgent") == true)
      assert(awful.tag.getproperty(tags[1][2], "urgent_count") == 1)
      return true
    end
  end,

  -- Step 2: when switching to tag 2, it should not be urgend anymore.
  function(count)
    if count == 1 then
      -- Setup: switch to tag.
      os.execute('xdotool key super+2')
    else
      assert(awful.tag.getproperty(tags[1][2], "urgent") == false)
      assert(awful.tag.getproperty(tags[1][2], "urgent_count") == 0)
      return true
    end
  end,

  -- Step 3: tag 2 should not be urgent, but switched to.
  function(count)
    if count == 1 then  -- Setup.
      local urgent_cb_done = false

      -- Select first tag.
      os.execute('xdotool key super+1')

      awful.rules.rules = {
        { rule = { class = "XTerm" },
        properties = { tag = tags[1][2], focus = true, switchtotag = true } },
      }
      awful.util.spawn("xterm")
    else
      if urgent_cb_done then
        assert(awful.tag.getproperty(tags[1][2], "urgent") == false)
        assert(awful.tag.getproperty(tags[1][2], "urgent_count") == 0)
        assert(awful.tag.selectedlist()[1] == tags[1][2])
        assert(awful.tag.selectedlist()[2] == nil)
        return true
      else
        assert(awful.tag.selectedlist()[1] == tags[1][1])
        assert(awful.tag.selectedlist()[2] == nil)
      end
    end
  end,

}


-- Setup timer/timeout to limit waiting for signal and quitting awesome.
-- This would be common for all tests.
local t = timer({timeout=0.1})
local wait=50
local step=1
local step_count=0
t:connect_signal("timeout", function() timer.delayed_call(function()
  io.flush()  -- for "tail -f".
  step_count = step_count + 1
  local step_as_string = step..'/'..#steps..' (@'..step_count..')'

  -- Call the current step's function.
  local success, result = pcall(steps[step], step_count)

  if not success then
    io.stderr:write('Error: running function for step '
      ..step_as_string..': '..tostring(result)..'!\n')
    t:stop()
    return  -- keep awesome open on error.

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
    error("Step "..step_as_string.." failed.")
    assert(false)

  else
    wait = wait-1
    -- io.stderr:write('wait:'..wait.."\n")
    if wait > 0 then
      t:again()
      return
    else
      io.stderr:write("Error: timeout waiting for signal in step "
        ..step_as_string..".\n")
      t:stop()
      return  -- keep awesome open on error.
    end
  end
  awesome.quit()
end) end)
t:start()
