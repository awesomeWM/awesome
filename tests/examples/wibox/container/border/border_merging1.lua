--DOC_GEN_IMAGE --DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")
local beautiful = require( "beautiful" )

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

local l = wibox.layout {
    spacing         = 100,
    forced_num_cols = 2,
    forced_num_rows = 2,
    homogeneous     = true,
    expand          = true,
    layout          = wibox.layout.grid.vertical
}

--DOC_HIDE_END

for _, side in ipairs { "top", "bottom", "left", "right" } do
    l:add(wibox.widget {
        {
            text   = side .. " = true",
            valign = "center",
            align  = "center",
            widget = wibox.widget.textbox
        },
        border_merging = {
            -- This is the equaivalent "left = true,". "side" is the loop
            -- variable.
            [side] = true
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
        widget = wibox.container.border
    })
end


--DOC_HIDE_START

parent:add(l)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
