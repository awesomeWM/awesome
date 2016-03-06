--- Test for awful.widget.watch

local runner = require("_runner")
local watch = require("awful.widget.watch")

local callbacks_done = 0

local steps = {
    function(count)
        if count == 1 then
            watch(
                "echo hi", 0.1,
                function(widget, stdout, stderr, exitreason, exitcode)
                    assert(widget == "i_am_widget_mock", widget)
                    assert(stdout == "hi\n", stdout)
                    assert(stderr == "", stderr)
                    assert(exitreason == "exit", exitreason)
                    assert(exitcode == 0, exitcode)
                    callbacks_done = callbacks_done + 1
                end,
                "i_am_widget_mock"
            )
        end
        if callbacks_done > 1 then  -- timer fired at least twice
            return true
        end
    end
}
runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
