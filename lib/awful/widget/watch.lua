---------------------------------------------------------------------------
--- Watch widget.
-- Here is an example of simple temperature widget which will update each 15
-- seconds implemented in two different ways.
-- The first, simpler one, will just display the return command output
-- (so output is stripped by shell commands).
-- In the other example `sensors` returns to the widget its full output
-- and it's trimmed in the widget callback function:
--
--     211             mytextclock,
--     212             wibox.widget.textbox('  |  '),
--     213             -- one way to do that:
--     214             awful.widget.watch('bash -c "sensors | grep temp1"', 15),
--     215             -- another way:
--     216             awful.widget.watch('sensors', 15, function(widget, stdout)
--     217               for line in stdout:gmatch("[^\r\n]+") do
--     218                 if line:match("temp1") then
--     219                   widget:set_text(line)
--     220                   return
--     221                 end
--     222               end
--     223             end),
--     224             s.mylayoutbox,
--
-- ![Example screenshot](../images/awful_widget_watch.png)
--
-- @author Benjamin Petrenko
-- @author Yauheni Kirylau
-- @copyright 2015, 2016 Benjamin Petrenko, Yauheni Kirylau
-- @classmod awful.widget.watch
---------------------------------------------------------------------------

local setmetatable = setmetatable
local textbox = require("wibox.widget.textbox")
local timer = require("gears.timer")
local spawn = require("awful.spawn")

local watch = { mt = {} }

--- Create a textbox that shows the output of a command and updates it at a
-- given time interval.
--
-- @tparam string|table command The command.
--
-- @tparam[opt=5] integer interval The time interval at which the textbox
-- will be updated.
--
-- @tparam[opt] function callback The function that will be called after
-- the command exited successfully.  The default is to update the textbox:
--     function(widget, stdout, stderr, exitreason, exitcode)
--         widget:set_text(stdout)
--     end
-- @tparam widget callback.widget Base widget instance.
-- @tparam string callback.stdout Output on stdout.
-- @tparam string callback.stderr Output on stderr.
-- @tparam string callback.exitreason Exit Reason.
-- The reason can be "exit" or "signal".
-- @tparam integer callback.exitcode Exit code.
-- For "exit" reason it's the exit code.
-- For "signal" reason — the signal causing process termination.
--
-- @tparam[opt=wibox.widget.textbox()] widget base_widget Base widget.
--
-- @return The widget used by this watch
function watch.new(command, interval, callback, base_widget)
    interval = interval or 5
    base_widget = base_widget or textbox()
    callback = callback or function(widget, stdout, stderr)
        widget:set_text(stdout)
    end
    callback_error = callback_error or function(widget, stdout, stderr, exitcode)
        widget:set_text(stderr)
    end
    local t = timer { timeout = interval }
    t:connect_signal("timeout", function()
        t:stop()
        spawn.easy_async(command, function(stdout, stderr, exitreason, exitcode)
          callback(base_widget, stdout, stderr, exitreason, exitcode)
          t:again()
        end)
    end)
    t:start()
    t:emit_signal("timeout")
    return base_widget
end

function watch.mt.__call(_, ...)
    return watch.new(...)
end

return setmetatable(watch, watch.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
