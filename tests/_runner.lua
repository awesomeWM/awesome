local timer = require("gears.timer")
local awful = require("awful")
local gtable = require("gears.table")

local runner = {
    quit_awesome_on_error = os.getenv('TEST_PAUSE_ON_ERRORS') ~= '1',
}

local verbose = os.getenv('VERBOSE') == '1'

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
        io.stderr:write("Error: run_steps() was never called.\n")
        if not runner.quit_awesome_on_error then
            io.stderr:write("Keeping awesome open...\n")
            return  -- keep awesome open on error.
        end
        awesome.quit()
    end
end)

runner.step_kill_clients = function(step)
    if step == 1 then
        for _,c in ipairs(client.get()) do
            c:kill()
        end
    end
    if #client.get() == 0 then
        return true
    end
end

--- Print a message if verbose mode is enabled.
-- @tparam string message The message to print.
function runner.verbose(message)
    if verbose then
        io.stderr:write(message .. "\n")
    end
end

--- When using run_direct(), this function indicates that the test is now done.
-- @tparam[opt=nil] string message An error message explaining the test failure, if it failed.
function runner.done(message)
    if message then
        io.stderr:write("Error: " .. message .. "\n")
        if not runner.quit_awesome_on_error then
            io.stderr:write("Keeping awesome open...\n")
            return
        end
    end

    local client_count = #client.get()
    if client_count > 0 then
        io.stderr:write(string.format(
            "NOTE: there were %d clients left after the test.\n", client_count))

        -- Remove any clients.
        for _,c in ipairs(client.get()) do
            c:kill()
        end
    end

    if not message then
        io.stderr:write("Test finished successfully.\n")
    end
    awesome.quit()
end

--- This function is called to indicate that a test does not use the run_steps()
-- facility, but instead runs something else directly.
function runner.run_direct()
    assert(not running, "API abuse: Test was started twice")
    running = true
end

--- Start some step-wise tests. The given steps are called in order until all
-- succeeded. Each step is a function that can return true/false to indicate
-- success/failure, but can also return nothing if it needs to be called again
-- later.
function runner.run_steps(steps, options)
    options = gtable.crush({
        kill_clients=true,
        wait_per_step=2,  -- how long to wait per step in seconds.
    }, options or {})
    -- Setup timer/timeout to limit waiting for signal and quitting awesome.
    local t = timer({timeout=0})
    local wait=options.wait_per_step / 0.1
    local step=1
    local step_count=0
    runner.run_direct()

    if options.kill_clients then
        -- Add a final step to kill all clients and wait for them to finish.
        -- Ref: https://github.com/awesomeWM/awesome/pull/1904#issuecomment-312793006
        steps[#steps + 1] = runner.step_kill_clients
    end

    t:connect_signal("timeout", function() timer.delayed_call(function()
        io.flush()  -- for "tail -f".
        step_count = step_count + 1
        local step_as_string = step..'/'..#steps..' (@'..step_count..')'
        runner.verbose(string.format('Running step %s..\n', step_as_string))

        -- Call the current step's function.
        local success, result = xpcall(function()
            return steps[step](step_count)
        end, debug.traceback)

        if not success then
            runner.done('running function for step '
                        ..step_as_string..': '..tostring(result)..'!')
            t:stop()
        elseif result then
            -- true: test succeeded.
            if step < #steps then
                -- Next step.
                step = step+1
                step_count = 0
                wait = options.wait_per_step / 0.1
                t.timeout = 0
                t:again()
            else
                -- All steps finished, we are done.
                runner.done()
            end
        elseif result == false then
            runner.done("Step "..step_as_string.." failed (returned false).")
        else
            -- No result yet, run this step again.
            wait = wait-1
            if wait > 0 then
                t.timeout = 0.1
                t:again()
            else
                runner.done("timeout waiting for signal in step "
                            ..step_as_string..".")
                t:stop()
            end
        end
    end) end)
    t:start()
end

return runner

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
