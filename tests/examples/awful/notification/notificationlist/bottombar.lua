--DOC_GEN_IMAGE
--DOC_NO_USAGE
require("_date") --DOC_HIDE
require("_default_look") --DOC_HIDE
local awful     = require("awful") --DOC_HIDE
local gears     = require("gears") --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE
local naughty   = require("naughty") --DOC_HIDE

screen[1]._resize {width = 640, height = 240} --DOC_HIDE

--DOC_HIDE Give some context, otherwise it doesn't look like a screen
local c = client.gen_fake {hide_first=true} --DOC_HIDE
c:geometry { x = 50, y = 45, height = 100, width = 250} --DOC_HIDE
c._old_geo = {c:geometry()} --DOC_HIDE
c:set_label("A client") --DOC_HIDE
c:emit_signal("request::titlebars", "rules", {})--DOC_HIDE

beautiful.notification_icon_size         = 48   --DOC_HIDE
beautiful.notification_action_label_only = true --DOC_HIDE

--DOC_NEWLINE

    -- This awful.wibar will be placed at the bottom and contain the notifications.
    local notif_wb = awful.wibar {
        position = "bottom",
        height   = 48,
        visible  = #naughty.active > 0,
    }

--DOC_NEWLINE

    notif_wb:setup {
        nil,
        {
            base_layout = wibox.widget {
                spacing_widget = wibox.widget {
                    orientation = "vertical",
                    span_ratio  = 0.5,
                    widget      = wibox.widget.separator,
                },
                forced_height = 30,
                spacing       = 3,
                layout        = wibox.layout.flex.horizontal
            },
            widget_template = wibox.template {
                {
                    naughty.widget.icon,
                    {
                        naughty.widget.title,
                        naughty.widget.message,
                        {
                            layout = wibox.widget {
                                -- Adding the `wibox.widget` allows to share a
                                -- single instance for all spacers.
                                spacing_widget = wibox.widget {
                                    orientation = "vertical",
                                    span_ratio  = 0.9,
                                    widget      = wibox.widget.separator,
                                },
                                spacing = 3,
                                layout  = wibox.layout.flex.horizontal
                            },
                            widget = naughty.list.widgets,
                        },
                        layout = wibox.layout.align.vertical
                    },
                    spacing = 10,
                    fill_space = true,
                    layout  = wibox.layout.fixed.horizontal
                },
                margins = 5,
                widget  = wibox.container.margin
            },
            widget = naughty.list.notifications,
        },
        -- Add a button to dismiss all notifications, because why not.
        {
            {
                text   = "Dismiss all",
                halign = "center",
                valign = "center",
                widget = wibox.widget.textbox
            },
            buttons = gears.table.join(
                awful.button({ }, 1, function() naughty.destroy_all_notifications() end)
            ),
            forced_width       = 75,
            shape              = gears.shape.rounded_bar,
            shape_border_width = 1,
            shape_border_color = beautiful.bg_highlight,
            widget = wibox.container.background
        },
        layout = wibox.layout.align.horizontal
    }

--DOC_NEWLINE

    -- We don't want to have that bar all the time, only when there is content.
    naughty.connect_signal("property::active", function()
        notif_wb.visible = #naughty.active > 0
    end)


--DOC_HIDE The delayed make sure the legacy popup gets disabled in time
gears.timer.run_delayed_calls_now()--DOC_HIDE

for i=1, 3 do --DOC_HIDE
    naughty.notification { --DOC_HIDE
        title = "A notification "..i, --DOC_HIDE
        text = "Be notified! "..i, --DOC_HIDE
        icon = i%2 == 1 and beautiful.awesome_icon, --DOC_HIDE
        timeout = 999, --DOC_HIDE
        actions = { --DOC_HIDE
            naughty.action { --DOC_HIDE
                name = "Accept "..i, --DOC_HIDE
                icon = beautiful.awesome_icon, --DOC_HIDE
            }, --DOC_HIDE
            naughty.action { --DOC_HIDE
                name = "Refuse", --DOC_HIDE
                icon = beautiful.awesome_icon, --DOC_HIDE
            }, --DOC_HIDE
        } --DOC_HIDE
    } --DOC_HIDE
end --DOC_HIDE


require("gears.timer").run_delayed_calls_now()
