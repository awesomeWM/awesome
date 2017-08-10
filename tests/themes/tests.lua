--- Simple script to load/test a theme.
-- This is used from tests/themes/run.sh.

local awful = require("awful")

local steps = {
    function(count)
        if count <= 2 then
            awful.spawn("xterm")
        elseif #client.get() == 2 then
            return true
        end
    end,
}

require("_runner").run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
