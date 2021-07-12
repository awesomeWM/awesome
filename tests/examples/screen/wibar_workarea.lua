--DOC_GEN_IMAGE

screen[1]._resize {x = 0, width = 640, height = 480} --DOC_HIDE

screen._add_screen {x = 820, width = 640, height = 480, y = -22} --DOC_HIDE

local awful = { --DOC_HIDE
    wibar = require("awful.wibar"), --DOC_HIDE
    tag = require("awful.tag"), --DOC_HIDE
    tag_layout = require("awful.layout.suit.tile") --DOC_HIDE
} --DOC_HIDE

local screen1_wibar = awful.wibar {
    position        = "top",
    update_workarea = true,
    height          = 24,
    screen          = screen[1],
}

--DOC_NEWLINE

local screen2_wibar = awful.wibar {
    position        = "top",
    update_workarea = false,
    height          = 24,
    screen          = screen[2],
}

return { --DOC_HIDE
    factor                = 2   , --DOC_HIDE
    show_boxes            = true, --DOC_HIDE
    draw_wibars           = {screen1_wibar, screen2_wibar}, --DOC_HIDE
    display_screen_info   = false, --DOC_HIDE
    draw_struts           = true, --DOC_HIDE
} --DOC_HIDE
