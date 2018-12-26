-- This file contains all global backward compatibility workarounds for the
-- Core API changes.
local gtimer = require("gears.timer")
local util   = require("awful.util" )
local spawn  = require("awful.spawn")
local gdebug = require("gears.debug")

local capi = {root = root}

function timer(...) -- luacheck: ignore
    gdebug.deprecate("gears.timer", {deprecated_in=4})
    return gtimer(...)
end

util.spawn = function(...)
   gdebug.deprecate("awful.spawn", {deprecated_in=4})
   return spawn.spawn(...)
end

util.spawn_with_shell = function(...)
   gdebug.deprecate("awful.spawn.with_shell", {deprecated_in=4})
   return spawn.with_shell(...)
end

util.pread = function()
    gdebug.deprecate("Use io.popen() directly or look at awful.spawn.easy_async() "
            .. "for an asynchronous alternative", {deprecated_in=4})
    return ""
end

-- Allow properties to be set on the root object. This helps to migrate some
-- capi function to an higher level Lua implementation.
do
    local root_props, root_object = {}, {}

    capi.root.set_newindex_miss_handler(function(_,key,value)
        if root_object["set_"..key] then
            root_object["set_"..key](value)
        elseif not root_object["get_"..key] then
            root_props[key] = value
        else
            -- If there is a getter, but no setter, then the property is read-only
            error("Cannot set '" .. tostring(key) .. " because it is read-only")
        end
    end)

    capi.root.set_index_miss_handler(function(_,key)
        if root_object["get_"..key] then
            return root_object["get_"..key]()
        else
            return root_props[key]
        end
    end)

    root._private = {}
    root.object = root_object
    assert(root.object == root_object)
end
