--DOC_GEN_IMAGE
local parent  = ... --DOC_HIDE --DOC_NO_USAGE
local naughty = { --DOC_HIDE
    list         = {actions = require("naughty.list.actions")}, --DOC_HIDE
    notification = require("naughty.notification"), --DOC_HIDE
    action       = require("naughty.action") --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

    local notif = naughty.notification { --DOC_HIDE
        title   = "A notification", --DOC_HIDE
        message = "This notification has actions!", --DOC_HIDE
        actions = { --DOC_HIDE
            naughty.action { --DOC_HIDE
                name = "Accept", --DOC_HIDE
                icon = beautiful.awesome_icon, --DOC_HIDE
            }, --DOC_HIDE
            naughty.action { --DOC_HIDE
                name = "Refuse", --DOC_HIDE
                icon = beautiful.awesome_icon, --DOC_HIDE
            }, --DOC_HIDE
            naughty.action { --DOC_HIDE
                name = "Ignore", --DOC_HIDE
                icon = beautiful.awesome_icon, --DOC_HIDE
            }, --DOC_HIDE
        } --DOC_HIDE
    } --DOC_HIDE

--DOC_NEWLINE

parent:add( wibox.container.background(--DOC_HIDE
    wibox.widget {
        notification = notif,
        forced_width = 250, --DOC_HIDE
        base_layout = wibox.widget {
            spacing        = 3,
            spacing_widget = wibox.widget {
                orientation = "vertical",
                widget      = wibox.widget.separator,
            },
            layout         = wibox.layout.flex.horizontal
        },
        widget_template = {
            {
                {
                    {
                        id            = "icon_role",
                        forced_height = 16,
                        forced_width  = 16,
                        widget        = wibox.widget.imagebox
                    },
                    {
                        id     = "text_role",
                        widget = wibox.widget.textbox
                    },
                    spacing = 5,
                    layout = wibox.layout.fixed.horizontal
                },
                id = "background_role",
                widget             = wibox.container.background,
            },
            margins = 4,
            widget  = wibox.container.margin,
        },
        widget = naughty.list.actions,
    }
,beautiful.bg_normal)) --DOC_HIDE
