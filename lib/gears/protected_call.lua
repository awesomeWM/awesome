---------------------------------------------------------------------------
-- Safely call a function and handle errors using `gears.debug`.
--
-- This is a `pcall`/`xpcall` wrapper. All it does is to integrate into the
-- AwesomeWM-wide error handling and logging.
--
-- @author Uli Schlachter
-- @copyright 2016 Uli Schlachter
-- @utillib gears.protected_call
---------------------------------------------------------------------------

local gdebug = require("gears.debug")
local tostring = tostring
local traceback = debug.traceback
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local xpcall = xpcall

local protected_call = {}

function protected_call._error_handler(err)
    gdebug.print_error(traceback("Error during a protected call: " .. tostring(err), 2))
end

function protected_call._handle_result(success, ...)
    if success then
        return ...
    end
end

local do_pcall
if not select(2, xpcall(function(a) return a end, error, true)) then
    -- Lua 5.1 doesn't support arguments in xpcall :-(
    do_pcall = function(func, ...)
        local args = { ... }
        return protected_call._handle_result(xpcall(function()
            return func(unpack(args))
        end, protected_call._error_handler))
    end
else
    do_pcall = function(func, ...)
        return protected_call._handle_result(xpcall(func, protected_call._error_handler, ...))
    end
end

--- Call a function in protected mode and handle error-reporting.
-- If the function call succeeds, all results of the function are returned.
-- Otherwise, an error message is printed and nothing is returned.
-- @tparam function func The function to call
-- @param ... Arguments to the function
-- @return The result of the given function, or nothing if an error occurred.
-- @staticfct gears.protected_call
function protected_call.call(func, ...)
    return do_pcall(func, ...)
end

local pcall_mt = {}
function pcall_mt:__call(...)
    return do_pcall(...)
end

return setmetatable(protected_call, pcall_mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
