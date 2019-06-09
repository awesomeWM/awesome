--DOC_GEN_IMAGE
local parent    = ... --DOC_HIDE_ALL
local naughty = {
    widget = { icon = require("naughty.widget.icon")},
    notification = require("naughty.notification"),
}
local wibox = require("wibox")
local beautiful = require("beautiful")

local notif = naughty.notification {
    title = "A notification",
    text = "This notification has actions!",
    icon = beautiful.awesome_icon,
    actions = {
        ["Accept"] = function() end,
        ["Refuse"] = function() end,
        ["Ignore"] = function() end,
    }
}

local icons = {}

for _, strategy in ipairs {"resize", "scale", "center" } do
    table.insert(icons, wibox.widget {
        {
            {
                resize_strategy = strategy,
                notification    = notif,
                widget          = naughty.widget.icon,
            },
            bg     = beautiful.bg_normal,
            widget =  wibox.container.background
        },
        valign = "top",
        halign = "left",
        widget = wibox.container.place
    })
end

parent:add(
    wibox.widget {
        {
            markup = "<b>resize:</b>",
            widget = wibox.widget.textbox,
        },
        {
            markup = "<b>scale:</b>",
            widget = wibox.widget.textbox,
        },
        {
            markup = "<b>center:</b>",
            widget = wibox.widget.textbox,
        },
        icons[1],
        icons[2],
        icons[3],
        forced_num_rows = 2,
        forced_num_cols = 3,
        spacing = 5,
        widget = wibox.layout.grid,
    }
)
