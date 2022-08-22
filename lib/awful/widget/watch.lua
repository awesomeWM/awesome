---------------------------------------------------------------------------
--- Execute a command at a set interval and display its output.
--
-- Here is an example of simple temperature widget which will update each 15
-- seconds implemented in two different ways.
-- The first, simpler one, will just display the return command output
-- (so output is stripped by shell commands).
-- In the other example `sensors` returns to the widget its full output
-- and it's trimmed in the widget callback function:
--
--    -- one way to do that:
--    local w = awful.widget.watch('bash -c "sensors | grep temp1"', 15)
--
--    -- another way:
--    local w = awful.widget.watch('sensors', 15, function(widget, stdout)
--        for line in stdout:gmatch("[^\r\n]+") do
--            if line:match("temp1") then
--                widget:set_text(line)
--                return
--            end
--        end
--    end)
--
-- ![Example screenshot](../images/awful_widget_watch.png)
--
-- Here is the most basic usage:
--
-- @DOC_wibox_awidget_defaults_watch_EXAMPLE@
--
-- @author Benjamin Petrenko
-- @author Yauheni Kirylau
-- @copyright 2015, 2016 Benjamin Petrenko, Yauheni Kirylau
-- @widgetmod awful.widget.watch
-- @supermodule wibox.widget.base
---------------------------------------------------------------------------

local setmetatable = setmetatable
local textbox = require("wibox.widget.textbox")
local timer = require("gears.timer")
local spawn = require("awful.spawn")

local watch = { mt = {} }

--- Create a textbox that shows the output of a command
-- and updates it at a given time interval.
--
-- @tparam string|table command The command.
--
-- @tparam[opt=5] integer timeout The time interval at which the textbox
-- will be updated.
--
-- @tparam[opt] function callback The function that will be called after
-- the command output will be received. it is shown in the textbox.
-- Defaults to:
--     function(widget, stdout, stderr, exitreason, exitcode)
--         widget:set_text(stdout)
--     end
-- @tparam wibox.widget callback.widget Base widget instance.
-- @tparam string callback.stdout Output on stdout.
-- @tparam string callback.stderr Output on stderr.
-- @tparam string callback.exitreason Exit Reason.
-- The reason can be "exit" or "signal".
-- @tparam integer callback.exitcode Exit code.
-- For "exit" reason it's the exit code.
-- For "signal" reason — the signal causing process termination.
--
-- @tparam[opt=wibox.widget.textbox()] wibox.widget base_widget Base widget.
--
-- @return The widget used by this watch.
-- @return Its gears.timer.
-- @constructorfct awful.widget.watch
function watch.new(command, timeout, callback, base_widget)
    timeout = timeout or 5
    base_widget = base_widget or textbox()
    callback = callback or function(widget, stdout, stderr, exitreason, exitcode) -- luacheck: no unused args
        widget:set_text(stdout)
    end
    local t = timer { timeout = timeout }
    t:connect_signal("timeout", function()
        t:stop()
        spawn.easy_async(command, function(stdout, stderr, exitreason, exitcode)
          callback(base_widget, stdout, stderr, exitreason, exitcode)
          t:again()
        end)
    end)
    t:start()
    t:emit_signal("timeout")
    return base_widget, t
end

function watch.mt.__call(_, ...)
    return watch.new(...)
end

--@DOC_object_COMMON@

return setmetatable(watch, watch.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
