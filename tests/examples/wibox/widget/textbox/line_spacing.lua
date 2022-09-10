--DOC_GEN_IMAGE --DOC_HIDE

--DOC_HIDE_START
local parent = ...
local wibox  = require("wibox")
local beautiful = require("beautiful")

local widget = function(inner)
    return wibox.widget {
        {
            inner,
            margins = 5,
            widget = wibox.container.margin,
        },
        border_width = 1,
        border_color = beautiful.border_color,
        widget = wibox.container.background,
    }
end

local l = wibox.layout.grid.vertical(4)
--DOC_HIDE_END

for _, spacing in ipairs {0.0, 0.1, 0.5, 0.9, 1, 1.5, 2.0, 2.5} do
    local text = "This text shas a line\nspacing of "..tostring(spacing).. "\nunits."
    --DOC_NEWLINE
    l:add( --DOC_HIDE
    widget{
        text                = text,
        font                = "sans 10",
        line_spacing_factor = spacing,
        widget              = wibox.widget.textbox,
    }
    ) --DOC_HIDE
end
--DOC_HIDE_START

parent:add(l)


-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
