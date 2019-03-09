---------------------------------------------------------------------------
--- Remote control module allowing usage of awesome-client.
--
-- @author Julien Danjou &lt;julien@danjou.info&gt;
-- @copyright 2009 Julien Danjou
-- @module awful.remote
---------------------------------------------------------------------------

-- Grab environment we need
local load = loadstring or load -- luacheck: globals loadstring (compatibility with Lua 5.1)
local tostring = tostring
local ipairs = ipairs
local table = table
local type = type

local lgi = require("lgi")
local GLib = lgi.GLib
local Gio = lgi.Gio
local GObject = lgi.GObject

local code_type = Gio.DBusArgInfo()
code_type.ref_count = 1
code_type.name = "code"
code_type.signature = "s"

local result_type = Gio.DBusArgInfo()
result_type.ref_count = 1
result_type.name = "result"
result_type.signature = "s"

local eval_method = Gio.DBusMethodInfo()
eval_method.ref_count = 1
eval_method.name = "Eval"
eval_method.in_args = { code_type }
eval_method.out_args = { result_type }

local interface_info = Gio.DBusInterfaceInfo()
interface_info.ref_count = 1
interface_info.name = "org.awesomewm.awful.Remote"
interface_info.methods = { eval_method }

local function method_call(_conn, _sender, _obj, _interface, _method, parameters, invocation)
    -- This must be "Eval", because that's the only method we have
    local code = parameters.value[1]

    local f, e = load(code)
    if not f then
        invocation:return_dbus_error("org.awesomewm.awful.Remote.InvalidCode", e)
        return
    end
    local results = { pcall(f) }
    if not table.remove(results, 1) then
        invocation:return_dbus_error("org.awesomewm.awful.Remote.ExecutionError", results[1])
        return
    end
    local types = ""
    local retvals = {}
    local retval_types = {}
    for i, v in ipairs(results) do
        local t = type(v)
        local dbus_type
        if t == "boolean" then
            dbus_type = "b"
            table.insert(retvals, v)
        elseif t == "number" then
            dbus_type = "d"
            table.insert(retvals, v)
        else
            dbus_type = "s"
            table.insert(retvals, tostring(v))
        end

        types = types .. dbus_type

        local info = Gio.DBusArgInfo()
        info.ref_count = 1
        info.name = "result-" .. tostring(i)
        info.signature = dbus_type
        table.insert(retval_types, info)
    end

    -- We are lying about our return arguments so that GDBus does not notice
    -- that we are lying.
    eval_method.out_args = retval_types
    invocation:return_value(GLib.Variant("(" .. types .. ")", retvals))
    eval_method.out_args = { result_type }
end

local function on_bus_acquire(conn, _name)
    conn:register_object("/", interface_info, GObject.Closure(method_call))
    conn:register_object("/org/awesomewm/awful/Remote", interface_info, GObject.Closure(method_call))
end

Gio.bus_own_name(Gio.BusType.SESSION, "org.awesomewm.awful",
    Gio.BusNameOwnerFlags.NONE, GObject.Closure(on_bus_acquire), nil, nil)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
