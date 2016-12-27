local runner = require("_runner")
local awful = require("awful")

local calls_done = 0

local function dbus_callback(data)
    assert(data.member == "Ping")
    calls_done = calls_done + 1
end

dbus.request_name("session", "org.awesomewm.test")

-- Yup, we had a bug that made the following not work
dbus.connect_signal("org.awesomewm.test", dbus_callback)
dbus.disconnect_signal("org.awesomewm.test", dbus_callback)
dbus.connect_signal("org.awesomewm.test", dbus_callback)

for _=1, 2 do
    awful.spawn({
                "dbus-send",
                "--dest=org.awesomewm.test",
                "--type=method_call",
                "/",
                "org.awesomewm.test.Ping",
                "string:foo"
            })
end

runner.run_steps({ function()
    if calls_done >= 2 then
        return true
    end
end })

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
