--DOC_GEN_IMAGE --DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")
local beautiful = require( "beautiful" )

parent.spacing = 50

local function generic_widget(text, margins)
    return wibox.widget {
        {
            {
                {
                    id     = "text",
                    align  = "center",
                    valign = "center",
                    text   = text,
                    widget = wibox.widget.textbox
                },
                margins = 10,
                widget  = wibox.container.margin,
            },
            border_width = 3,
            border_color = beautiful.border_color,
            bg = beautiful.bg_normal,
            widget = wibox.container.background
        },
        margins = margins or 5,
        widget  = wibox.container.margin,
    }
end

--DOC_HIDE_END

for _, expand in ipairs { false, true } do
    local w = wibox.widget {
        {
            text          = "expand_corners = " .. (expand and "true" or "false"),
            valign        = "center",
            align         = "center",
            widget        = wibox.widget.textbox
        },
        border_widgets = {
            top_left     = generic_widget("top_left"),
            top          = generic_widget("top"),
            top_right    = generic_widget("top_right"),
            right        = generic_widget("right"),
            bottom_right = generic_widget("bottom_right"),
            bottom       = generic_widget("bottom"),
            bottom_left  = generic_widget("bottom_left"),
            left         = generic_widget("left"),
        },
        expand_corners = expand,
        widget         = wibox.container.border
    }
    parent:add(wibox.container.place(w)) --DOC_HIDE
end

----DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
