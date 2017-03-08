local timer = require("gears.timer")
local awful = require("awful")
local gtable = require("gears.table")

local runner = {
    quit_awesome_on_error = os.getenv('TEST_PAUSE_ON_ERRORS') ~= '1',
}

-- Helpers.

--- Add some rules to awful.rules.rules, after the defaults.
local default_rules = gtable.clone(awful.rules.rules)
runner.add_to_default_rules = function(r)
    awful.rules.rules = gtable.clone(default_rules)
    table.insert(awful.rules.rules, r)
end

-- Was the runner started already?
local running = false

-- This is used if a test causes errors before starting the runner
timer.start_new(1, function()
    if not running then
        io.stderr:write("Error: run_steps() was never called\n")
        if not runner.quit_awesome_on_error then
            io.stderr:write("Keeping awesome open...\n")
            return  -- keep awesome open on error.
        end
        awesome.quit()
    end
end)

runner.run_steps = function(steps)
    -- Setup timer/timeout to limit waiting for signal and quitting awesome.
    -- This would be common for all tests.
    local t = timer({timeout=0})
    local wait=20
    local step=1
    local step_count=0
    assert(not running, "run_steps() was called twice")
    running = true
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
                t.timeout = 0
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
                t.timeout = 0.1
                t:again()
            else
                io.stderr:write("Error: timeout waiting for signal in step "
                                ..step_as_string..".\n")
                t:stop()
            end
            return
        end
        -- Remove any clients.
        for _,c in ipairs(client.get()) do
            c:kill()
        end
        if success and result then
            io.stderr:write("Test finished successfully\n")
        end
        awesome.quit()
    end) end)
    t:start()
end

return runner

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
