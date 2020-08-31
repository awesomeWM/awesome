--DOC_GEN_IMAGE --DOC_HEADER --DOC_HIDE --DOC_HIDE_ALL
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = {--DOC_HIDE
    widget = {launcher = require("awful.widget.launcher")} --DOC_HIDE
}--DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

    local mymainmenu = {}

    local mylauncher = --DOC_HIDE
    awful.widget.launcher {
        image = beautiful.awesome_icon,
        menu  = mymainmenu
    }

assert(mylauncher)
mylauncher.forced_height = 24 --DOC_HIDE
mylauncher.forced_width = 24 --DOC_HIDE

parent:add(mylauncher) --DOC_HIDE
