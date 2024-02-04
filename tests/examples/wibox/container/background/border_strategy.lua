--DOC_GEN_IMAGE --DOC_HIDE_START
local parent    = ...
local wibox     = require("wibox")
local gears     = { shape = require("gears.shape") }
local beautiful = require("beautiful")

local l = wibox.layout {
    forced_width    = 640,
    spacing         = 5,
    forced_num_cols = 2,
    homogeneous     = false,
    expand          = false,
    layout          = wibox.layout.grid.vertical
}

--DOC_HIDE_END

for k, strategy in ipairs { "none", "inner" } do
    --DOC_HIDE_START
    local r = k*2

    l:add_widget_at(wibox.widget {
        markup = "<b>border_strategy = \"".. strategy .."\"</b>",
        widget = wibox.widget.textbox,
    }, r, 1, 1, 2)
    --DOC_HIDE_END

    for idx, width in ipairs {0, 1, 3, 10 } do
        local w = wibox.widget {
            {
                {
                    text          = "border_width = "..width,
                    valign        = "center",
                    align         = "center",
                    widget        = wibox.widget.textbox
                },
                border_color    = beautiful.bg_normal,
                border_width    = width,
                border_strategy = strategy,
                shape           = gears.shape.rounded_rect,
                widget          = wibox.container.background
            },
            widget = wibox.container.place
        }

        l:add_widget_at(w, r+1, idx, 1, 1) --DOC_HIDE
    end
    --DOC_HIDE_END
end
--DOC_HIDE_START

parent:add(l)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
