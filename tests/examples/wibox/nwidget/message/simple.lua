--DOC_GEN_IMAGE
local parent    = ... --DOC_HIDE
local naughty = { --DOC_HIDE
    widget = { message = require("naughty.widget.message")}, --DOC_HIDE
    notification = require("naughty.notification")} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

    local notif = naughty.notification {
        title   = "A notification",
        message = "This notification no actions!",
        icon    = beautiful.awesome_icon,
    }

--DOC_NEWLINE

parent:add( --DOC_HIDE
    wibox.widget {
        notification = notif,
        widget       = naughty.widget.message,
    }
) --DOC_HIDE
