--- Test for awful.widget.watch

local runner = require("_runner")
local watch = require("awful.widget.watch")

local callbacks_done = 0

local steps = {
    function(count)
        if count == 1 then
            watch(
                "sh -c 'echo hi; printf stderr >&2'", 0.1,
                function(widget, stdout, stderr)
                    assert(widget == "i_am_widget_mock", widget)
                    assert(stdout == "hi\n", tostring(stdout))
                    assert(stderr == "stderr\n", tostring(stderr))
                    callbacks_done = callbacks_done + 1
                end,
                "i_am_widget_mock"
            )
        end
        if callbacks_done >= 2 then
            return true
        end
    end,
}
runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
