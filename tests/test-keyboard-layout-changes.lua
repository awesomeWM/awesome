-- Test for bug #1494: Using xmodmap freezes awesome since it re-queries the
-- keyboard layout many, many times. (xmodmap applies each change on its own)

local runner = require("_runner")
local spawn = require("awful.spawn")
local GLib = require("lgi").GLib

local done
local timer = GLib.Timer()

local steps = {
    function(count)
        if count == 1 then
            -- POSIX allows us to use awk
            local cmd = "awk 'BEGIN { for(i=1; i<=1000;i++) print \"keycode 107 = parenleft\" }' | xmodmap -"
            spawn.easy_async({"sh", "-c", cmd}, function()
                awesome.sync()
                done = true
            end)
        end
        if done then
            -- Apply some limit on how long awesome may need to process 'things'
            return timer:elapsed() < 5
        end
    end
}
runner.run_steps(steps)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
