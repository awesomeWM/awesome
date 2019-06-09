--DOC_GEN_IMAGE
local parent  = ... --DOC_HIDE --DOC_NO_USAGE
local naughty = { --DOC_HIDE
    list         = {actions = require("naughty.list.actions")}, --DOC_HIDE
    notification = require("naughty.notification"), --DOC_HIDE
    action       = require("naughty.action") --DOC_HIDE
} --DOC_HIDE
local gears = {shape = require("gears.shape")} --DOC_HIDE
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
                selected = true, --DOC_HIDE
            }, --DOC_HIDE
            naughty.action { --DOC_HIDE
                name = "Ignore", --DOC_HIDE
                icon = beautiful.awesome_icon, --DOC_HIDE
            }, --DOC_HIDE
        } --DOC_HIDE
    } --DOC_HIDE

--DOC_NEWLINE

parent:add( wibox.container.margin(--DOC_HIDE
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
        style = {
            underline_normal             = false,
            underline_selected           = true,
            shape_normal                 = gears.shape.octogon,
            shape_selected               = gears.shape.hexagon,
            shape_border_width_normal    = 2,
            shape_border_width_selected  = 4,
            icon_size_normal             = 16,
            icon_size_selected           = 24,
            shape_border_color_normal    = "#0000ff",
            shape_border_color_selected  = "#ff0000",
            bg_normal                    = "#ffff00",
            bg_selected                  = "#00ff00",
        },
        forced_height = beautiful.get_font_height(beautiful.font) * 2.5,
        widget = naughty.list.actions,
    }
,0,0,5,5)) --DOC_HIDE
