--DOC_GEN_IMAGE
--DOC_HIDE_START
local awful     = require("awful")
local wibox     = require("wibox")

screen._track_workarea = true
screen[1]._resize {width = 360, height = 60}
screen._add_screen {x = 0, width = 360, height = 64, y = 72} --DOC_HIDE
screen._add_screen {x = 0, width = 360, height = 64, y = 144} --DOC_HIDE
screen._add_screen {x = 372, width = 64, height = 210, y = 0} --DOC_HIDE
screen._add_screen {x = 444, width = 64, height = 210, y = 0} --DOC_HIDE
screen._add_screen {x = 516, width = 64, height = 210, y = 0} --DOC_HIDE

--DOC_HIDE_END

for s, align in ipairs { "left", "centered", "right" } do
    awful.wibar {
        position = "top",
        screen   = screen[s],
        stretch  = false,
        width    = 196,
        align    = align,
        widget   = {
            text   = align,
            align  = "center",
            widget = wibox.widget.textbox
        },
    }
end

--DOC_NEWLINE

for s, align in ipairs { "top", "centered", "bottom" } do
    awful.wibar {
        position = "left",
        screen   = screen[s+3],
        stretch  = false,
        height   = 128,
        align    = align,
        widget   = {
            {
                text   = align,
                align  = "center",
                widget = wibox.widget.textbox
            },
            direction = "east",
            widget    = wibox.container.rotate
        },
    }
end


--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

