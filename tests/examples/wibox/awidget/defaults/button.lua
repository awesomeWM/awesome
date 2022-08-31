--DOC_HIDE_START --DOC_GEN_IMAGE --DOC_NO_USAGE
local parent = ...
local awful = {
    button = require("awful.button"),
    widget = {button = require("awful.widget.button")}
}
local beautiful = require("beautiful")

    local button =
    --DOC_HIDE_END
    awful.widget.button {
        image   = beautiful.awesome_icon,
        buttons = {
            awful.button({}, 1, nil, function ()
                print("Mouse was clicked")
            end)
        }
    }

--DOC_HIDE_START
button.forced_height = 24
button.forced_width = 24

parent:add(button)
