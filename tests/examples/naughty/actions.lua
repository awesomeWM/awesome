--DOC_NO_USAGE
--DOC_HIDE_ALL
-- local naughty = require("naughty")

dbus.notify_send(
    --[[data]] {
        member = "Notify",
    },
    --[[app_name]]    "Notification demo",
    --[[replaces_id]] nil,
    --[[icon]] "",
    --[[title]] "You got a message!",
    --[[text]] "This is a message from above.\nAwesomeWM is your faith.",
    {"Accept", "Dismiss", "Forward"},
    --[[hints]] {},
    --[[expire]] 5
)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
