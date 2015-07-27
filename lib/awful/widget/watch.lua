---------------------------------------------------------------------------
--- Watch widget.
--
-- @release @AWESOME_VERSION@
-- @classmod awful.widget.watch
---------------------------------------------------------------------------

local setmetatable = setmetatable
local textbox = require("wibox.widget.textbox")
local timer = require("gears.timer")

local watch = { mt = {} }

--- Create a textbox that shows the output of a command and updates it at a given time interval.
--
-- @param command The command.
-- @param timeout The time interval at which the textbox will be updated.
-- @param f (optional) The function that will be applied to the output before it is shown in the textbox.
-- @return A textbox widget.
function watch.new(command, timeout, f)
    f = f or (function(s) return s end)
    local w = textbox()
    local t = timer { timeout = timeout }
    t:connect_signal("timeout", function()
        fh = assert(io.popen(command, "r"))
        w:set_text(f(fh:read("*l")))
        fh:close()
    end)
    t:start()
    t:emit_signal("timeout")
    return w
end

function watch.mt:__call(...)
    return watch.new(...)
end

return setmetatable(watch, watch.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
