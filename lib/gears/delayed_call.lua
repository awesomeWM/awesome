---------------------------------------------------------------------------
--- Do some operation 'later'
--
-- @author Uli Schlachter
-- @copyright 2015 Uli Schlachter
-- @classmod gears.delayed_call
---------------------------------------------------------------------------

local capi = { awesome = awesome }
local ipairs = ipairs
local table = table
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local protected_call = require("gears.protected_call")

local module = {}

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
-- @function gears.delayed_call
function module.delayed_call(callback, ...)
    assert(type(callback) == "function", "callback must be a function, got: " .. type(callback))
    table.insert(delayed_calls, { callback, ... })
end

return setmetatable(module, { __call = function(_, ...) return module.delayed_call(...) end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
