--DOC_GEN_IMAGE --DOC_HIDE --DOC_NO_USAGE --DOC_NO_DASH
local parent = ... --DOC_HIDE
local gears = require("gears") --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

    local w = wibox.widget {

        -- Get the current cliently focused name.
        gears.connection {
            source_class         = client,
            signal               = "focused",
            source_property      = "name",
            destination_property = "text",
        },

        widget = wibox.widget.textbox
    }

parent:add(w) --DOC_HIDE
