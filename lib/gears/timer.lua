---------------------------------------------------------------------------
--- Class to execute code at specific intervals.
--
-- @usage
--    -- Create a widget and update its content using the output of a shell
--    -- command every 10 seconds:
--    local mybatterybar = wibox.widget {
--        {
--            min_value    = 0,
--            max_value    = 100,
--            value        = 0,
--            paddings     = 1,
--            border_width = 1,
--            forced_width = 50,
--            border_color = "#0000ff",
--            id           = "mypb",
--            widget       = wibox.widget.progressbar,
--        },
--        {
--            id           = "mytb",
--            text         = "100%",
--            widget       = wibox.widget.textbox,
--        },
--        layout      = wibox.layout.stack,
--        set_battery = function(self, val)
--            self.mytb.text  = tonumber(val).."%"
--            self.mypb.value = tonumber(val)
--        end,
--    }
--
--    gears.timer {
--        timeout   = 10,
--        call_now  = true,
--        autostart = true,
--        callback  = function()
--            -- You should read it from `/sys/class/power_supply/` (on Linux)
--            -- instead of spawning a shell. This is only an example.
--            awful.spawn.easy_async(
--                {"sh", "-c", "acpi | sed -n 's/^.*, \([0-9]*\)%/\1/p'"},
--                function(out)
--                    mybatterybar.battery = out
--                end
--            )
--        end
--    }
--
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
-- @coreclassmod gears.timer
---------------------------------------------------------------------------

local capi = { awesome = awesome }
local ipairs = ipairs
local pairs = pairs
local setmetatable = setmetatable
local table = table
local tonumber = tonumber
local traceback = debug.traceback
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local glib = require("lgi").GLib
local object = require("gears.object")
local protected_call = require("gears.protected_call")
local gdebug = require("gears.debug")
local gmath = require("gears.math")

--- Timer objects. This type of object is useful when triggering events repeatedly.
--
-- The timer will emit the "timeout" signal every N seconds, N being the timeout
-- value. Note that a started timer will not be garbage collected. Call `:stop`
-- before dropping the last reference to prevent leaking the object.
--
-- @tfield number timeout Interval in seconds to emit the timeout signal.
--   Can be any value, including floating point ones (e.g. `1.5` seconds).
-- @tfield boolean started Read-only boolean field indicating if the timer has been
--   started.
-- @table timer

--- Emitted when the timer is started.
-- @signal start

--- Emitted when the timer is stopped.
-- @signal stop

--- Emitted when `timeout` seconds have elapsed.
--
-- This will be emitted repeatedly, unless `single_shot` has been set to `true`
-- for the timer.
-- @signal timeout

local timer = { mt = {} }

--- Start the timer.
-- @method start
-- @emits start
function timer:start()
    if self.data.source_id ~= nil then
        gdebug.print_error(traceback("timer already started"))
        return
    end
    local timeout_ms = gmath.round(self.data.timeout * 1000)
    self.data.source_id = glib.timeout_add(glib.PRIORITY_DEFAULT, timeout_ms, function()
        protected_call(self.emit_signal, self, "timeout")
        return true
    end)
    self:emit_signal("start")
end

--- Stop the timer.
--
-- Does nothing if the timer isn't running.
--
-- @method stop
-- @emits stop
function timer:stop()
    if self.data.source_id == nil then
        return
    end
    glib.source_remove(self.data.source_id)
    self.data.source_id = nil
    self:emit_signal("stop")
end

--- Restart the timer.
-- This is equivalent to stopping the timer if it is running and then starting
-- it.
-- @method again
-- @emits start
-- @emits stop
function timer:again()
    if self.data.source_id ~= nil then
        self:stop()
    end
    self:start()
end

--- The timer is started.
--
-- For this to be `true` by default, pass `autostart` to the constructor.
--
-- @property[opt=false] started
-- @tparam boolean started
-- @see start
-- @see stop

--- The timer timeout value.
--
-- @property timeout
-- @tparam number timeout
-- @propemits true false

local timer_instance_mt = {
    __index = function(self, property)
        if property == "timeout" then
            return self.data.timeout
        elseif property == "started" then
            return self.data.source_id ~= nil
        end

        return timer[property]
    end,

    __newindex = function(self, property, value)
        if property == "timeout" then
            self.data.timeout = tonumber(value)
            self:emit_signal("property::timeout", value)
        end
    end
}

--- Create a new timer object.
--
-- `call_now` only takes effect when a `callback` is provided. `single_shot`,
-- on the other hand, also stops signals connected to the `timeout` signal.
--
-- Specifying a function `func` as `args.callback` is equivalent to calling
-- `:connect_signal(func)` on the timer object.
--
-- @tparam table args Arguments.
-- @tparam number args.timeout Timeout in seconds (e.g. `1.5`).
-- @tparam[opt=false] boolean args.autostart Automatically start the timer.
-- @tparam[opt=false] boolean args.call_now Call the callback at timer creation.
-- @tparam[opt] function args.callback Callback function to connect to the
--  "timeout" signal.
-- @tparam[opt=false] boolean args.single_shot Run only once then stop.
-- @treturn timer
-- @constructorfct gears.timer
function timer.new(args)
    args = args or {}
    local ret = object()

    ret.data = { timeout = 0 } --TODO v5 rename to ._private
    setmetatable(ret, timer_instance_mt)

    for k, v in pairs(args) do
        ret[k] = v
    end

    if args.autostart then
        ret:start()
    end

    if args.callback then
        if args.call_now then
            args.callback()
        end
        ret:connect_signal("timeout", args.callback)
    end

    if args.single_shot then
        ret:connect_signal("timeout", function() ret:stop() end)
    end

    return ret
end

--- Create a simple timer for calling the `callback` function continuously.
--
-- This is a small wrapper around `gears.timer`, that creates a timer based on
-- `callback`.
-- The timer will run continuously and call `callback` every `timeout` seconds.
-- It is stopped when `callback` returns `false`, when `callback` throws an
-- error or when the `:stop()` method is called on the return value.
--
-- @tparam number timeout Timeout in seconds (e.g. 1.5).
-- @tparam function callback Function to run.
-- @treturn timer The new timer object.
-- @staticfct gears.timer.start_new
-- @see gears.timer.weak_start_new
function timer.start_new(timeout, callback)
    local t = timer.new({ timeout = timeout })
    t:connect_signal("timeout", function()
        local cont = protected_call(callback)
        if not cont then
            t:stop()
        end
    end)
    t:start()
    return t
end

--- Create a simple timer for calling the `callback` function continuously.
--
-- This function is almost identical to `gears.timer.start_new`. The only
-- difference is that this does not prevent the callback function from being
-- garbage collected.
-- In addition to the conditions in `gears.timer.start_new`,
-- this timer will also stop if `callback` was garbage collected since the
-- previous run.
--
-- @tparam number timeout Timeout in seconds (e.g. 1.5).
-- @tparam function callback Function to start.
-- @treturn timer The new timer object.
-- @staticfct gears.timer.weak_start_new
-- @see gears.timer.start_new
function timer.weak_start_new(timeout, callback)
    local indirection = setmetatable({}, { __mode = "v" })
    indirection.callback = callback
    return timer.start_new(timeout, function()
        local cb = indirection.callback
        if cb then
            return cb()
        end
    end)
end

local delayed_calls = {}

--- Run all pending delayed calls now. This function should best not be used at
-- all, because it means that less batching happens and the delayed calls run
-- prematurely.
-- @staticfct gears.timer.run_delayed_calls_now
function timer.run_delayed_calls_now()
    for _, callback in ipairs(delayed_calls) do
        protected_call(unpack(callback))
    end
    delayed_calls = {}
end

--- Call the given function at the end of the current GLib event loop iteration.
-- @tparam function callback The function that should be called
--@param ... Arguments to the callback function
-- @staticfct gears.timer.delayed_call
function timer.delayed_call(callback, ...)
    assert(type(callback) == "function", "callback must be a function, got: " .. type(callback))
    table.insert(delayed_calls, { callback, ... })
end

capi.awesome.connect_signal("refresh", timer.run_delayed_calls_now)

function timer.mt.__call(_, ...)
    return timer.new(...)
end

--@DOC_object_COMMON@

return setmetatable(timer, timer.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
