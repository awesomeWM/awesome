---------------------------------------------------------------------------
--- Text clock widget.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @classmod wibox.widget.textclock
---------------------------------------------------------------------------

local setmetatable = setmetatable
local os = os
local textbox = require("wibox.widget.textbox")
local timer = require("gears.timer")
local glib = require("lgi").GLib
local DateTime = glib.DateTime
local TimeZone = glib.TimeZone

local textclock = { mt = {} }

--- This lowers the timeout so that it occurs "correctly". For example, a timeout
-- of 60 is rounded so that it occurs the next time the clock reads ":00 seconds".
local function calc_timeout(real_timeout)
    return real_timeout - os.time() % real_timeout
end

--- Create a textclock widget. It draws the time it is in a textbox.
--
-- @tparam[opt=" %a %b %d, %H:%M "] string format The time format.
-- @tparam[opt=60] number timeout How often update the time (in seconds).
-- @tparam[opt=local timezone] string timezone The timezone to use,
--   e.g. "Z" for UTC, "±hh:mm" or "Europe/Amsterdam". See
--   https://developer.gnome.org/glib/stable/glib-GTimeZone.html#g-time-zone-new.
-- @treturn table A textbox widget.
-- @function wibox.widget.textclock
function textclock.new(format, timeout, timezone)
    format = format or " %a %b %d, %H:%M "
    timeout = timeout or 60
    timezone = timezone and TimeZone.new(timezone) or TimeZone.new_local()

    local w = textbox()
    local t
    function w._private.textclock_update_cb()
        w:set_markup(DateTime.new_now(timezone):format(format))
        t.timeout = calc_timeout(timeout)
        t:again()
        return true -- Continue the timer
    end
    t = timer.weak_start_new(timeout, w._private.textclock_update_cb)
    t:emit_signal("timeout")
    return w
end

function textclock.mt:__call(...)
    return textclock.new(...)
end

--@DOC_widget_COMMON@

--@DOC_object_COMMON@

return setmetatable(textclock, textclock.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
