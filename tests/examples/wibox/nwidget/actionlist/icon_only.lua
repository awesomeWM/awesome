--DOC_GEN_IMAGE
local parent    = ... --DOC_HIDE --DOC_NO_USAGE
local naughty = { --DOC_HIDE
    list         = {actions = require("naughty.list.actions")}, --DOC_HIDE
    notification = require("naughty.notification"), --DOC_HIDE
    action       = require("naughty.action") --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

    local notif = naughty.notification {
        title   = "A notification",
        message = "This notification has actions!",
        actions = {
            naughty.action {
                name = "Accept",
                icon = beautiful.awesome_icon,
                icon_only = true,
            },
            naughty.action {
                name = "Refuse",
                icon = beautiful.awesome_icon,
                icon_only = true,
            },
            naughty.action {
                name = "Ignore",
                icon = beautiful.awesome_icon,
                icon_only = true,
            },
        }
    }

--DOC_NEWLINE

parent:add( wibox.container.background(--DOC_HIDE
    wibox.widget {
        notification = notif,
        widget = naughty.list.actions,
    }
,beautiful.bg_normal)) --DOC_HIDE
