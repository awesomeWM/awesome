--DOC_GEN_IMAGE --DOC_HIDE_START
local parent    = ...
local wibox     = require("wibox")
local gears     = { shape = require("gears.shape"), color = require("gears.color") }
local beautiful = require("beautiful")

parent.spacing = 5

--DOC_HIDE_END

local colors = {
    beautiful.bg_normal,
    "#00ff00",
    gears.color {
        type  = "linear",
        from  = { 0 , 20 },
        to    = { 20, 0  },
        stops = {
            { 0, "#0000ff" },
            { 1, "#ff0000" }
        },
    },
}

--DOC_NEWLINE

for _, color in ipairs(colors) do
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
        border_color         = color,
        border_width         = 3,
        stretch_vertically   = true,
        stretch_horizontally = true,
        shape                = gears.shape.rounded_rect,
        widget               = wibox.container.background
    }

    parent:add(w) --DOC_HIDE
end

--DOC_HIDE_START

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
