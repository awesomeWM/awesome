--DOC_GEN_IMAGE --DOC_HIDE
local awful = { --DOC_HIDE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {clienticon =require("awful.widget.clienticon"), --DOC_HIDE
              tasklist = require("awful.widget.tasklist")} --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

screen[1]._resize {width = 320, height = 240} --DOC_HIDE
screen._add_screen {x = 330, y = 0, width = 320, height = 240} --DOC_HIDE

local p = awful.popup { --DOC_HIDE
    widget = wibox.widget { --DOC_HIDE
        text = "Parent wibox", --DOC_HIDE
        forced_width = 100, --DOC_HIDE
        forced_height = 50, --DOC_HIDE
        widget = wibox.widget.textbox --DOC_HIDE
    }, --DOC_HIDE
    border_color = "#777777", --DOC_HIDE
    border_width = 2, --DOC_HIDE
    ontop        = true, --DOC_HIDE
    screen       = screen[1], --DOC_HIDE
    placement    = awful.placement.top, --DOC_HIDE
} --DOC_HIDE
p:_apply_size_now() --DOC_HIDE

    local p2 = awful.popup {
        widget = wibox.widget {
            text   = "A popup",
            forced_height = 100,
            widget = wibox.widget.textbox
        },
        border_color        = "#777777",
        border_width        = 2,
        preferred_positions = "right",
        preferred_anchors   = {"front", "back"},
    }
    p2:move_next_to(p) --DOC_HIDE

local p3 = awful.popup { --DOC_HIDE
    widget = wibox.widget { --DOC_HIDE
        text = "Parent wibox2", --DOC_HIDE
        forced_width = 100, --DOC_HIDE
        forced_height = 50, --DOC_HIDE
        widget = wibox.widget.textbox --DOC_HIDE
    }, --DOC_HIDE
    border_color = "#777777", --DOC_HIDE
    border_width = 2, --DOC_HIDE
    ontop        = true, --DOC_HIDE
    screen       = screen[2], --DOC_HIDE
    placement    = awful.placement.bottom, --DOC_HIDE
} --DOC_HIDE
p3:_apply_size_now() --DOC_HIDE

    local p4 = awful.popup {
        widget = wibox.widget {
            text   = "A popup2",
            forced_height = 100,
            widget = wibox.widget.textbox
        },
        border_color        = "#777777",
        border_width        = 2,
        preferred_positions = "right",
        screen              = screen[2], --DOC_HIDE
        preferred_anchors   = {"front", "back"},
    }
    p4:move_next_to(p3) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
