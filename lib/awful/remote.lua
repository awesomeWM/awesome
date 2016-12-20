---------------------------------------------------------------------------
--- Remote control module allowing usage of awesome-client.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @module awful.remote
---------------------------------------------------------------------------

-- Grab environment we need
require("awful.dbus")
local load = loadstring or load -- luacheck: globals loadstring (compatibility with Lua 5.1)
local tostring = tostring
local ipairs = ipairs
local table = table
local unpack = unpack or table.unpack -- luacheck: globals unpack (compatibility with Lua 5.1)
local dbus = dbus
local type = type

if dbus then
    dbus.connect_signal("org.awesomewm.awful.Remote", function(data, code)
        if data.member == "Eval" then
            local f, e = load(code)
            if f then
                local results = { f() }
                local retvals = {}
                for _, v in ipairs(results) do
                    local t = type(v)
                    if t == "boolean" then
                        table.insert(retvals, "b")
                        table.insert(retvals, v)
                    elseif t == "number" then
                        table.insert(retvals, "d")
                        table.insert(retvals, v)
                    else
                        table.insert(retvals, "s")
                        table.insert(retvals, tostring(v))
                    end
                end
                return unpack(retvals)
            elseif e then
                return "s", e
            end
        end
    end)
end

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
