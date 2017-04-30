-- Test if client's c:swap() corrupts the Lua stack

local runner = require("_runner")
local spawn = require("awful.spawn")

local todo = 2
local had_error = false

local function wait_a_bit(count)
    if todo == 0 or count == 5 then
        return true
    end
end
runner.run_steps({
    function()
        local err = spawn.with_line_callback(
            { os.getenv("build_dir") ..  "/test-gravity" },
            {
                exit = function(what, code)
                    assert(what == "exit", what)
                    assert(code == 0, "Exit code was " .. code)
                    todo = todo - 1
                end,
                stderr = function(line)
                    had_error = true
                    print("Read on stderr: " .. line)
                end,
                stdout = function(line)
                    if line == "SUCCESS" then
                        todo = todo - 1
                    elseif line:sub(1, 5) ~= "LOG: " then
                        had_error = true
                        print("Read on stdout: " .. line)
                    end
                end
            })
        assert(type(err) ~= "string", err)
        return true
    end,
    -- Buy the external program some time to finish
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    wait_a_bit,
    function()
        if todo == 0 then
            assert(not had_error, "Some error occurred, see above")
            return true
        end
    end
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
