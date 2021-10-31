--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local l = ...
local wibox = require("wibox")
local beautiful = require("beautiful")

--DOC_HIDE_END
    local w = wibox.widget {
        text   = "This is some text\nover\nmultiple lines!",
        widget = wibox.widget.textbox,
    }
--DOC_HIDE_START

l:add(wibox.widget{
    w,
    margins =  1,
    forced_height = 60,
    color = beautiful.bg_normal,
    widget = wibox.container.margin,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
