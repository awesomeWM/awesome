---------------------------------------------------------------------------
--- Text clock widget.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
-- @classmod awful.widget.textclock
---------------------------------------------------------------------------

local setmetatable = setmetatable
local os = os
local textbox = require("wibox.widget.textbox")
local timer = require("gears.timer")
local DateTime = require("lgi").GLib.DateTime

local textclock = { mt = {} }

--- This lowers the timeout so that it occurs "correctly". For example, a timeout
-- of 60 is rounded so that it occurs the next time the clock reads ":00 seconds".
local function calc_timeout(real_timeout)
    return real_timeout - os.time() % real_timeout
end

--- Create a textclock widget. It draws the time it is in a textbox.
--
-- @param format The time format. Default is " %a %b %d, %H:%M ".
-- @param timeout How often update the time. Default is 60.
-- @return A textbox widget.
function textclock.new(format, timeout)
    local format = format or " %a %b %d, %H:%M "
    local timeout = timeout or 60

    local w = textbox()
    local t
    function w._textclock_update_cb()
        w:set_markup(DateTime.new_now_local():format(format))
        t.timeout = calc_timeout(timeout)
        t:again()
        return true -- Continue the timer
    end
    t = timer.weak_start_new(timeout, w._textclock_update_cb)
    t:emit_signal("timeout")
    return w
end

function textclock.mt:__call(...)
    return textclock.new(...)
end

return setmetatable(textclock, textclock.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
