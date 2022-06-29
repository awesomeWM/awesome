--DOC_HIDE_START --DOC_GEN_IMAGE
local parent = ...
local wibox = require("wibox")
local beautiful = require("beautiful")

parent.spacing = 10

parent:add(wibox.widget {
    layout = wibox.layout.fixed.vertical,
    spacing = 2,
    {
        markup = "state == <b>true</b>",
        widget = wibox.widget.textbox
    },
--DOC_HIDE_END
{
    {
        {
            text = "Hello, World!",
            widget = wibox.widget.textbox,
        },
        state = true,
        widget = wibox.container.conditional,
    },
    bg = beautiful.bg_normal,
    widget = wibox.container.background,
},
--DOC_HIDE_START
})

parent:add(wibox.widget {
    layout = wibox.layout.fixed.vertical,
    spacing = 2,
    {
        markup = "state == <b>false</b>",
        widget = wibox.widget.textbox
    },
--DOC_HIDE_END
{
    { --DOC_HIDE
    {
        {
            text = "Hello, World!",
            widget = wibox.widget.textbox,
        },
        state = false,
        widget = wibox.container.conditional,
    },
--DOC_HIDE_START
    strategy = "min",
    width = 100,
    height = 18,
    widget = wibox.container.constraint,
    },
--DOC_HIDE_END
    bg = beautiful.bg_normal,
    widget = wibox.container.background,
},
--DOC_HIDE_START
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
