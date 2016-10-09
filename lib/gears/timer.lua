---------------------------------------------------------------------------
--- Timer objects and functions.
--
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
-- @classmod gears.timer
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

--- Timer objects. This type of object is useful when triggering events repeatedly.
-- The timer will emit the "timeout" signal every N seconds, N being the timeout
-- value. Note that a started timer will not be garbage collected. Call `:stop`
-- to enable garbage collection.
-- @tfield number timeout Interval in seconds to emit the timeout signal.
--   Can be any value, including floating point ones (e.g. 1.5 seconds).
-- @tfield boolean started Read-only boolean field indicating if the timer has been
--   started.
-- @table timer

--- When the timer is started.
-- @signal .start

--- When the timer is stopped.
-- @signal .stop

--- When the timer had a timeout event.
-- @signal .timeout

local timer = { mt = {} }

--- Start the timer.
function timer:start()
    if self.data.source_id ~= nil then
        print(traceback("timer already started"))
        return
    end
    self.data.source_id = glib.timeout_add(glib.PRIORITY_DEFAULT, self.data.timeout * 1000, function()
        protected_call(self.emit_signal, self, "timeout")
        return true
    end)
    self:emit_signal("start")
end

--- Stop the timer.
function timer:stop()
    if self.data.source_id == nil then
        print(traceback("timer not started"))
        return
    end
    glib.source_remove(self.data.source_id)
    self.data.source_id = nil
    self:emit_signal("stop")
end

--- Restart the timer.
-- This is equivalent to stopping the timer if it is running and then starting
-- it.
function timer:again()
    if self.data.source_id ~= nil then
        self:stop()
    end
    self:start()
end

--- The timer is started.
-- @property started
-- @param boolean

--- The timer timeout value.
-- **Signal:** property::timeout
-- @property timeout
-- @param number

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
            self:emit_signal("property::timeout")
        end
    end
}

--- Create a new timer object.
-- @tparam table args Arguments.
-- @tparam number args.timeout Timeout in seconds (e.g. 1.5).
-- @treturn timer
-- @function gears.timer
timer.new = function(args)
    local ret = object()

    ret.data = { timeout = 0 }
    setmetatable(ret, timer_instance_mt)

    for k, v in pairs(args) do
        ret[k] = v
    end

    return ret
end

--- Create a timeout for calling some callback function.
-- When the callback function returns true, it will be called again after the
-- same timeout. If false is returned, no more calls will be done. If the
-- callback function causes an error, no more calls are done.
-- @tparam number timeout Timeout in seconds (e.g. 1.5).
-- @tparam function callback Function to run.
-- @treturn timer The timer object that was set up.
-- @see timer.weak_start_new
-- @function gears.timer.start_new
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

--- Create a timeout for calling some callback function.
-- This function is almost identical to `timer.start_new`. The only difference
-- is that this does not prevent the callback function from being garbage
-- collected. After the callback function was collected, the timer returned
-- will automatically be stopped.
-- @tparam number timeout Timeout in seconds (e.g. 1.5).
-- @tparam function callback Function to start.
-- @treturn timer The timer object that was set up.
-- @see timer.start_new
-- @function gears.timer.weak_start_new
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
capi.awesome.connect_signal("refresh", function()
    for _, callback in ipairs(delayed_calls) do
        protected_call(unpack(callback))
    end
    delayed_calls = {}
end)

--- Call the given function at the end of the current main loop iteration
-- @tparam function callback The function that should be called
-- @param ... Arguments to the callback function
-- @function gears.timer.delayed_call
function timer.delayed_call(callback, ...)
    assert(type(callback) == "function", "callback must be a function, got: " .. type(callback))
    table.insert(delayed_calls, { callback, ... })
end

function timer.mt.__call(_, ...)
    return timer.new(...)
end

return setmetatable(timer, timer.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
