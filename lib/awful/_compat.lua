-- This file contains all global backward compatibility workarounds for the
-- Core API changes.
local gtimer = require("gears.timer")
local util   = require("awful.util")
local spawn  = require("awful.spawn")
local gdebug = require("gears.debug")
local gprop  = require("gears.object.properties")

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

-- root.bottons() used to be a capi function. However this proved confusing
-- as rc.lua used `awful.button` and `root.buttons()` used capi.button. There
-- was a little documented hack to "flatten" awful.button into a pair of
-- capi.button. A consequence of this, beside being ugly, was that it was
-- de-facto read-only due the confusion related the difference between the
-- capi and "high level" format difference.

gprop._legacy_accessors(capi.root, "buttons", "_buttons", false, function(new_btns)
    return new_btns[1] and (
        type(new_btns[1]) == "button" or new_btns[1]._is_capi_button
    ) or false
end, true)
