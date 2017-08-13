--DOC_HIDE_ALL
--DOC_GEN_IMAGE
-- local naughty = require("naughty")

dbus.notify_send(
    --[[data]] {
        member = "Notify",
    },
    --[[app_name]]    "Notification demo",
    --[[replaces_id]] nil,
    --[[icon]] "",
    --[[title]] "Hello world!",
    --[[text]] "The notification content",
    --[[actions]] {},
    --[[hints]] {},
    --[[expire]] 5
)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
