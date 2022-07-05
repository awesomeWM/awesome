---------------------------------------------------------------------------
--- Display the time (and date) in a text box.
--
-- The `wibox.widget.textclock` widget is part of the Awesome WM's widget
-- system (see @{03-declarative-layout.md}).
--
-- This widget displays a text clock formatted by the
-- [GLib Date Time format](https://developer.gnome.org/glib/stable/glib-GDateTime.html#g-date-time-format)
-- and [GTimeZone](https://developer.gnome.org/glib/stable/glib-GTimeZone.html#g-time-zone-new).
--
-- The `wibox.widget.textclock` inherits from `wibox.widget.textbox`. It means
-- that, once created, the user will receive a derivated instance of
-- `wibox.widget.textbox` associated with a private `gears.timer` to manage
-- timed updates of the displayed clock.
--
-- Use a `wibox.widget.textclock`
-- ---
--
-- @DOC_wibox_widget_defaults_textclock_EXAMPLE@
--
-- Alternatively, you can declare the `textclock` widget using the
-- declarative pattern (Both codes are strictly equivalent):
--
-- @DOC_wibox_widget_declarative-pattern_textclock_EXAMPLE@
--
-- The GLib DateTime format
-- ---
--
-- The time displayed by the textclock widget can be formated by the GLib
-- DateTime format.
--
-- Here is a short list with commonly used format specifiers (extracted from
-- the Glib API references):
--
--@DOC_glib_timedate_format_COMMON@
--
-- You can read more on the GLib DateTime format in the
-- [GLib documentation](https://developer.gnome.org/glib/stable/glib-GDateTime.html#g-date-time-format).
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @widgetmod wibox.widget.textclock
-- @supermodule wibox.widget.textbox
---------------------------------------------------------------------------

local setmetatable = setmetatable
local os = os
local textbox = require("wibox.widget.textbox")
local timer = require("gears.timer")
local gtable = require("gears.table")
local glib = require("lgi").GLib
local DateTime = glib.DateTime
local TimeZone = glib.TimeZone

local textclock = { mt = {} }

local DateTime_new_now = DateTime.new_now

-- When $SOURCE_DATE_EPOCH and $SOURCE_DIRECTORY are both set, then this code is
-- most likely being run by the test runner. Ensure reproducible dates.
local source_date_epoch = tonumber(os.getenv("SOURCE_DATE_EPOCH"))
if source_date_epoch and os.getenv("SOURCE_DIRECTORY") then
    DateTime_new_now = function()
        return DateTime.new_from_unix_utc(source_date_epoch)
    end
end

--- Set the clock's format.
--
-- For information about the format specifiers, see
-- [the GLib docs](https://developer.gnome.org/glib/stable/glib-GDateTime.html#g-date-time-format).
-- @property format
-- @tparam string format The new time format. This can contain pango markup.

function textclock:set_format(format)
    self._private.format = format
    self:force_update()
end

function textclock:get_format()
    return self._private.format
end

--- Set the clock's timezone.
--
-- e.g. "Z" for UTC, "±hh:mm" or "Europe/Amsterdam". See
-- [GTimeZone](https://developer.gnome.org/glib/stable/glib-GTimeZone.html#g-time-zone-new).
-- @property timezone
-- @tparam string timezone

function textclock:set_timezone(tzid)
    self._private.tzid = tzid
    self._private.timezone = tzid and TimeZone.new(tzid)
    self:force_update()
end

function textclock:get_timezone()
    return self._private.tzid
end

--- Set the clock's refresh rate.
--
-- @property refresh
-- @tparam number refresh How often the clock is updated, in seconds

function textclock:set_refresh(refresh)
    self._private.refresh = refresh or self._private.refresh
    self:force_update()
end

function textclock:get_refresh()
    return self._private.refresh
end

--- Force a textclock to update now.
--
-- @noreturn
-- @method force_update
function textclock:force_update()
    self._timer:emit_signal("timeout")
end

--- This lowers the timeout so that it occurs "correctly". For example, a timeout
-- of 60 is rounded so that it occurs the next time the clock reads ":00 seconds".
local function calc_timeout(real_timeout)
    return real_timeout - os.time() % real_timeout
end

--- Create a textclock widget. It draws the time it is in a textbox.
--
-- @tparam[opt=" %a %b %d&comma; %H:%M "] string format The time [format](#format).
-- @tparam[opt=60] number refresh How often to update the time (in seconds).
-- @tparam[opt=local timezone] string timezone The [timezone](#timezone) to use.
-- @treturn table A textbox widget.
-- @constructorfct wibox.widget.textclock
local function new(format, refresh, tzid)
    local w = textbox()
    gtable.crush(w, textclock, true)

    w._private.format = format or " %a %b %d, %H:%M "
    w._private.refresh = refresh or 60
    w._private.tzid = tzid
    w._private.timezone = tzid and TimeZone.new(tzid)

    function w._private.textclock_update_cb()
        local str = DateTime_new_now(w._private.timezone or TimeZone.new_local()):format(w._private.format)
        if str == nil then
            require("gears.debug").print_warning("textclock: "
                    .. "g_date_time_format() failed for format "
                    .. "'" .. w._private.format .. "'")
        end
        w:set_markup(str)
        w._timer.timeout = calc_timeout(w._private.refresh)
        w._timer:again()
        return true -- Continue the timer
    end

    w._timer = timer.weak_start_new(refresh, w._private.textclock_update_cb)
    w:force_update()
    return w
end

function textclock.mt:__call(...)
    return new(...)
end

return setmetatable(textclock, textclock.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
