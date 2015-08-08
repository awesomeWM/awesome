---------------------------------------------------------------------------
--- Timer objects and functions.
--
-- @author Uli Schlachter
-- @copyright 2014 Uli Schlachter
-- @release @AWESOME_VERSION@
-- @module gears.timer
---------------------------------------------------------------------------

local capi = { awesome = awesome }
local ipairs = ipairs
local pairs = pairs
local setmetatable = setmetatable
local table = table
local tonumber = tonumber
local traceback = debug.traceback
local unpack = unpack or table.unpack -- v5.1: unpack, v5.2: table.unpack
local glib = require("lgi").GLib
local object = require("gears.object")

--- Timer objects. This type of object is useful when triggering events repeatedly.
-- The timer will emit the "timeout" signal every N seconds, N being the timeout
-- value.
-- @tfield number timeout Interval in seconds to emit the timeout signal.
--                        Can be any value, including floating point ones
--                        (e.g. 1.5 seconds).
-- @tfield boolean started Read-only boolean field indicating if the timer has been
--                         started.
-- @table timer

local timer = { mt = {} }

--- Start the timer.
function timer:start()
    if self.data.source_id ~= nil then
        print(traceback("timer already started"))
        return
    end
    self.data.source_id = glib.timeout_add(glib.PRIORITY_DEFAULT, self.data.timeout * 1000, function()
        local success, message = xpcall(function()
                self:emit_signal("timeout")
            end, function(err)
                print(debug.traceback("Error during executing timeout handler: "..tostring(err)))
            end)
        return true
    end)
end

--- Stop the timer.
function timer:stop()
    if self.data.source_id == nil then
        print(traceback("timer not started"))
        return
    end
    glib.source_remove(self.data.source_id)
    self.data.source_id = nil
end

--- Restart the timer.
function timer:again()
    if self.data.source_id ~= nil then
        self:stop()
    end
    self:start()
end

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
timer.new = function(args)
    local ret = object()

    ret:add_signal("property::timeout")
    ret:add_signal("timeout")

    ret.data = { timeout = 0 }
    setmetatable(ret, timer_instance_mt)

    for k, v in pairs(args) do
        ret[k] = v
    end

    return ret
end

local delayed_calls = {}
capi.awesome.connect_signal("refresh", function()
    for _, callback in ipairs(delayed_calls) do
        local success, message = xpcall(function()
                callback[1](unpack(callback, 2))
            end, function(err)
                print(debug.traceback("Error during delayed call: "..tostring(err), 2))
            end)
    end
    delayed_calls = {}
end)

--- Call the given function at the end of the current main loop iteration
-- @tparam function callback The function that should be called
-- @param ... Arguments to the callback function
function timer.delayed_call(callback, ...)
    assert(type(callback) == "function", "callback must be a function, got: " .. type(callback))
    table.insert(delayed_calls, { callback, ... })
end

function timer.mt:__call(...)
    return timer.new(...)
end

return setmetatable(timer, timer.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
