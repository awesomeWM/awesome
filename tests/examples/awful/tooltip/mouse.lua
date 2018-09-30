--DOC_GEN_IMAGE
--DOC_NO_USAGE
screen[1]._resize {width = 200, height = 50} --DOC_HIDE
screen.no_outline = true --DOC_HIDE
local awful = {tooltip = require("awful.tooltip")} --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

mouse.coords{x=50, y= 10} --DOC_HIDE
mouse.push_history() --DOC_HIDE

    local tt = awful.tooltip {
        text = "A tooltip!",
        visible = true,
    }

--DOC_NEWLINE

    tt.bg = beautiful.bg_normal

--DOC_NEWLINE
