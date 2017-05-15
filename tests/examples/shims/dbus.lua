local dbus = awesome._shim_fake_class()

function dbus.notify_send(data, appname, replaces_id, icon, title, text, actions, hints, expire)
    dbus.emit_signal(
        "org.freedesktop.Notifications",
        data,
        appname,
        replaces_id,
        icon,
        title,
        text,
        actions,
        hints,
        expire
    )
end

function dbus.request_name()
    -- Ignore
end

return dbus

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
