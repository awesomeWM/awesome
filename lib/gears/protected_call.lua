---------------------------------------------------------------------------
-- @author Uli Schlachter
-- @copyright 2016 Uli Schlachter
-- @module gears.protected_call
---------------------------------------------------------------------------

local gdebug = require("gears.debug")
local tostring = tostring
local traceback = debug.traceback
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local xpcall = xpcall

local protected_call = {}

local function error_handler(err)
    gdebug.print_error(traceback("Error during a protected call: " .. tostring(err), 2))
end

local function handle_result(success, ...)
    if success then
        return ...
    end
end

local do_pcall
if _VERSION <= "Lua 5.1" then
    -- Lua 5.1 doesn't support arguments in xpcall :-(
    do_pcall = function(func, ...)
        local args = { ... }
        return handle_result(xpcall(function()
            return func(unpack(args))
        end, error_handler))
    end
else
    do_pcall = function(func, ...)
        return handle_result(xpcall(func, error_handler, ...))
    end
end

--- Call a function in protected mode and handle error-reporting.
-- If the function call succeeds, all results of the function are returned.
-- Otherwise, an error message is printed and nothing is returned.
-- @tparam function func The function to call
-- @param ... Arguments to the function
-- @return The result of the given function, or nothing if an error occurred.
function protected_call.call(func, ...)
    return do_pcall(func, ...)
end

local pcall_mt = {}
function pcall_mt:__call(...)
    return do_pcall(...)
end

return setmetatable(protected_call, pcall_mt)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
