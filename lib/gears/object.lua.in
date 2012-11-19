---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2010 Uli Schlachter
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

local setmetatable = setmetatable
local pairs = pairs
local type = type
local error = error

-- gears.object
local object = { mt = {} }

-- Verify that obj is indeed a valid object as returned by new()
local function check(obj)
    if type(obj) ~= "table" or type(obj._signals) ~= "table" then
        error("add_signal() called on non-object")
    end
end

--- Find a given signal
-- @param obj The object to search in
-- @param name The signal to find
-- @param error_msg Error message for if the signal is not found
-- @return The signal table
local function find_signal(obj, name, error_msg)
    check(obj)
    if not obj._signals[name] then
        error("Trying to " .. error_msg .. " non-existent signal '" .. name .. "'")
    end
    return obj._signals[name]
end

--- Add a signal to an object. All signals must be added before they can be used.
-- @param name The name of the new signal.
function object:add_signal(name)
    check(self)
    if not self._signals[name] then
        self._signals[name] = {}
    end
end

--- Connect to a signal
-- @param name The name of the signal
-- @param func The callback to call when the signal is emitted
function object:connect_signal(name, func)
    local sig = find_signal(self, name, "connect to")
    sig[func] = func
end

--- Disonnect to a signal
-- @param name The name of the signal
-- @param func The callback that should be disconnected
function object:disconnect_signal(name, func)
    local sig = find_signal(self, name, "disconnect from")
    sig[func] = nil
end

--- Emit a signal
-- @param name The name of the signal
-- @param ... Extra arguments for the callback functions. Each connected
--            function receives the object as first argument and then any extra
--            arguments that are given to emit_signal()
function object:emit_signal(name, ...)
    local sig = find_signal(self, name, "emit")
    for func in pairs(sig) do
        func(self, ...)
    end
end

-- Returns a new object. You can call :emit_signal(), :disconnect_signal,
-- :connect_signal() and :add_signal() on the resulting object.
local function new()
    local ret = {}

    -- Copy all our global functions to our new object
    for k, v in pairs(object) do
        if type(v) == "function" then
            ret[k] = v
        end
    end

    ret._signals = {}

    return ret
end

function object.mt:__call(...)
    return new(...)
end

return setmetatable(object, object.mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
