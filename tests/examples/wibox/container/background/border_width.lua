--DOC_GEN_IMAGE --DOC_HIDE_START
local parent    = ...
local wibox     = require("wibox")
local gears     = { shape = require("gears.shape") }
local beautiful = require("beautiful")

parent.spacing = 5

--DOC_HIDE_END

for _, width in ipairs {0, 1, 3, 10 } do
    local w = wibox.widget {
        {
            {
                text          = "  Content  ",
                valign        = "center",
                align         = "center",
                widget        = wibox.widget.textbox
            },
            margins = 10,
            widget  = wibox.container.margin
        },
        border_color = beautiful.bg_normal,
        border_width = width,
        shape        = gears.shape.rounded_rect,
        widget       = wibox.container.background
    }

    parent:add(w) --DOC_HIDE
end

--DOC_HIDE_START

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
