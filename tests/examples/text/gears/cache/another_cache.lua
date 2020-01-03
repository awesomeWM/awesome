--DOC_GEN_OUTPUT --DOC_HIDE
local gears = require("gears") --DOC_HIDE

local function tostring_for_cache(obj)
    return obj[1]
end
local counter = 0
local wrapper_cache = gears.cache.new(function(arg)
    local kind = "cache object #" .. tostring(counter) .. " for " .. tostring(arg)
    counter = counter + 1
    return setmetatable({ kind }, { __tostring = tostring_for_cache })
end)
print(wrapper_cache:get("first"))
print(wrapper_cache:get("second"))
-- No new object since it already exists
print(wrapper_cache:get("first"))
print("forcing a garbage collect")
-- The GC can *always* clear the cache
collectgarbage("collect")
print(wrapper_cache:get("first"))
print(wrapper_cache:get("second"))
