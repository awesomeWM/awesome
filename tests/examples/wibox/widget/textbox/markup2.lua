--DOC_GEN_IMAGE --DOC_HIDE_START --DOC_NO_USAGE
local l = ...
local wibox = require("wibox")
local beautiful = require("beautiful")

--DOC_HIDE_END
    local w = wibox.widget {
        markup = "<span background='#ff0000' foreground='#0000ff'>Some</span>"..
          " nice <span foreground='#00ff00'>colors!</span>",
        widget = wibox.widget.textbox,
    }
--DOC_HIDE_START

l:add(wibox.widget{
    w,
    margins =  1,
    color = beautiful.bg_normal,
    widget = wibox.container.margin,
})

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
