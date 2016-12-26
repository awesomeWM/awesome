--- Tests for spawn + signals

local runner = require("_runner")

local done = false

local function exit_callback(cause, signal)
    assert(cause == "signal", cause)
    assert(signal == awesome.unix_signal.SIGKILL, signal)
    done = true
end
local pid = awesome.spawn({"/bin/sh", "-c", "sleep 60"}, false, false,
                          false, false, exit_callback)
awesome.kill(pid, awesome.unix_signal.SIGKILL)

runner.run_steps({
                 function()
                     if done then
                         return true
                     end
                 end
             })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
