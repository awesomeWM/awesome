-- Test if client's c:swap() corrupts the Lua stack

local runner = require("_runner")
local spawn = require("awful.spawn")

local had_exit, had_success
local had_error = false

local function check_done()
    if had_exit and had_success then
        if had_error then
            runner.done("Some error occurred, see above")
        else
            runner.done()
        end
    end
end

local err = spawn.with_line_callback(
    { "./test-gravity" },
    {
        exit = function(what, code)
            assert(what == "exit", what)
            assert(code == 0, "Exit code was " .. code)
            had_exit = true
            check_done()
        end,
        stderr = function(line)
            had_error = true
            print("Read on stderr: " .. line)
        end,
        stdout = function(line)
            if line == "SUCCESS" then
                had_success = true
                check_done()
            elseif line:sub(1, 5) ~= "LOG: " then
                had_error = true
                print("Read on stdout: " .. line)
            end
        end
    })

assert(type(err) ~= "string", err)
runner.run_direct()

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
