--DOC_GEN_IMAGE
--DOC_HIDE_START
local awful     = require("awful")
local wibox     = require("wibox")

screen._track_workarea = true
screen[1]._resize {width = 480, height = 60}
screen._add_screen {x = 0, width = 480, height = 64, y = 72} --DOC_HIDE

--DOC_HIDE_END

awful.wibar {
    position = "top",
    screen   = screen[1],
    stretch  = true,
    width    = 196,
    widget   = {
        text   = "stretched",
        halign = "center",
        widget = wibox.widget.textbox
    },
}

--DOC_NEWLINE

awful.wibar {
    position = "top",
    screen   = screen[2],
    stretch  = false,
    width    = 196,
    widget   = {
        text   = "not stretched",
        align  = "center",
        widget = wibox.widget.textbox
    },
}


--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

