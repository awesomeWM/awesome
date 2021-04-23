--DOC_GEN_IMAGE --DOC_HEADER --DOC_HIDE
local parent = ... --DOC_NO_USAGE --DOC_HIDE
local awful = {--DOC_HIDE
    button = require("awful.button"), --DOC_HIDE
    widget = {button = require("awful.widget.button")} --DOC_HIDE
}--DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

    local button = --DOC_HIDE
    awful.widget.button {
        image   = beautiful.awesome_icon,
        buttons = {
            awful.button({}, 1, nil, function ()
                print("Mouse was clicked")
            end)
        }
    }

button.forced_height = 24 --DOC_HIDE
button.forced_width = 24 --DOC_HIDE

parent:add(button) --DOC_HIDE
