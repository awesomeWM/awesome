--DOC_GEN_IMAGE --DOC_HEADER --DOC_HIDE
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = {--DOC_HIDE
    layout = require("awful.layout"), --DOC_HIDE
    widget = {layoutbox = require("awful.widget.layoutbox")} --DOC_HIDE
}--DOC_HIDE

    local button = --DOC_HIDE
    awful.widget.layoutbox {screen = 1}

button.forced_height = 24 --DOC_HIDE
button.forced_width = 24 --DOC_HIDE

parent:add(button) --DOC_HIDE
