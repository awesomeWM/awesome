local dbus = awesome._shim_fake_class()

function dbus.notify_send(...)
    dbus.emit_signal("org.freedesktop.Notifications", ...)
end

function dbus.request_name()
    -- Ignore
end

return dbus

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
