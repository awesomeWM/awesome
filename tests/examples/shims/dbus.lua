local lgi = require("lgi")
local Gio = lgi.Gio

local dbus = awesome._shim_fake_class()

-- Monkeypatch away the real dbus support
Gio.bus_own_name = function() end

--HACK it used to be an internal API, which made testing easy, but now it uses
-- GDBus, so this shims a small subset of its API and use some internal APIs
-- to access "private" methods. Note that it would be cleaner to implement an
-- high level (and testable) binding and use signals again.
local ndbus = nil

local invocation = {
    return_value = function() end
}

local function parameters_miss(t, k)
    if k == "get_child_value" then
        return function(idx) return t[idx-1] end
    end
end

function dbus.notify_send(_--[[appdata]], ...)
    ndbus = ndbus or require("naughty.dbus")

    ndbus._notif_methods.Notify(
        "sender",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify",
        setmetatable({value={...}}, {__index=parameters_miss}),
        invocation
    )

    -- Legacy API
    dbus.emit_signal("org.freedesktop.Notifications", ...)
end

function dbus.request_name()
    -- Ignore
end

return dbus

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
