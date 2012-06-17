---------------------------------------------------------------------------
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @release @AWESOME_VERSION@
---------------------------------------------------------------------------

-- Grab environment we need
require("awful.dbus")
local load = loadstring or load -- v5.1 - loadstring, v5.2 - load
local tostring = tostring
local ipairs = ipairs
local table = table
local unpack = unpack or table.unpack -- v5.1: unpack, v5.2: table.unpack
local dbus = dbus
local type = type

--- Remote control module allowing usage of awesome-client.
-- awful.remote

if dbus then
    dbus.connect_signal("org.naquadah.awesome.awful.Remote", function(data, code)
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
