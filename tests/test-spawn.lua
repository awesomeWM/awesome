--- Tests for spawn

local runner = require("_runner")
local spawn = require("awful.spawn")

local spawns_done = 0
local async_spawns_done = 0
local exit_yay, exit_snd = nil, nil

-- * Using spawn with array is already covered by the test client.
-- * spawn with startup notification is covered by test-spawn-snid.lua

local steps = {
    function()
        -- Test various error conditions. There are quite a number of them...
        local error_message

        error_message = spawn("this_does_not_exist_and_should_fail")
        assert(string.find(error_message, 'No such file or directory'), error_message)

        error_message = spawn({"this_does_not_exist_and_should_fail"})
        assert(string.find(error_message, 'No such file or directory'), error_message)

        error_message = spawn("foo '")
        assert(string.find(error_message, 'parse error: Text ended before matching quote was found'), error_message)

        error_message = spawn()
        assert(string.find(error_message, 'No command to execute'), error_message)

        error_message = spawn(" ")
        assert(string.find(error_message, 'Text was empty'), error_message)

        error_message = spawn("")
        assert(string.find(error_message, 'No command to execute'), error_message)

        error_message = spawn{}
        assert(string.find(error_message, 'There is nothing to execute'), error_message)

        return true
    end,

    function(count)
        if count == 1 then
            spawn.easy_async("echo yay", function(stdout)
                if stdout:match("yay") then
                    async_spawns_done = async_spawns_done + 1
                end
            end)
            spawn.easy_async_with_shell("true && echo yay", function(stdout)
                if stdout:match("yay") then
                    async_spawns_done = async_spawns_done + 1
                end
            end)
            local steps_yay = 0
            spawn.with_line_callback("echo yay", {
                                     stdout = function(line)
                                         assert(line == "yay", "line == '" .. tostring(line) .. "'")
                                         assert(steps_yay == 0)
                                         steps_yay = steps_yay + 1
                                     end,
                                     output_done = function()
                                         assert(steps_yay == 1)
                                         steps_yay = steps_yay + 1
                                         spawns_done = spawns_done + 1
                                     end,
                                     exit = function(reason, code)
                                         assert(reason == "exit")
                                         assert(exit_yay == nil)
                                         assert(code == 0)
                                         exit_yay = code
                                     end
                                 })

            local steps_count = 0
            local err_count = 0
            spawn.with_line_callback({ "sh", "-c", "printf line1\\\\nline2\\\\nline3 ; echo err >&2 ; exit 42" }, {
                                     stdout = function(line)
                                         assert(steps_count < 3)
                                         steps_count = steps_count + 1
                                         assert(line == "line" .. steps_count, "line == '" .. tostring(line) .. "'")
                                     end,
                                     stderr = function(line)
                                         assert(err_count == 0)
                                         err_count = err_count + 1
                                         assert(line == "err", "line == '" .. tostring(line) .. "'")
                                     end,
                                     output_done = function()
                                         assert(steps_count == 3)
                                         assert(err_count == 1)
                                         steps_count = steps_count + 1
                                         spawns_done = spawns_done + 1
                                     end,
                                     exit = function(reason, code)
                                         assert(reason == "exit")
                                         assert(exit_snd == nil)
                                         assert(code == 42)
                                         exit_snd = code
                                     end
                                 })
        end
        if spawns_done == 2 then
            assert(exit_yay == 0)
            assert(exit_snd == 42)
            assert(async_spawns_done == 2)
            return true
        end
    end
}

runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
