--DOC_GEN_IMAGE --DOC_HIDE_START
local parent    = ...
local wibox     = require("wibox")
local gears     = { shape = require("gears.shape"), color = require("gears.color") }

local l = wibox.layout {
    forced_width    = 640,
    spacing         = 5,
    forced_num_cols = 2,
    homogeneous     = false,
    expand          = false,
    layout          = wibox.layout.grid.vertical
}

--DOC_HIDE_END

local gradients = {
    gears.color {
        type  = "linear",
        from  = { 0, 0   },
        to    = { 0, 100 },
        stops = {
            { 0  , "#0000ff" },
            { 0.8, "#0000ff" },
            { 1  , "#ff0000" }
        }
    },
    gears.color {
        type = "radial",
        from  = { 30, 98, 20  },
        to    = { 30, 98, 120 },
        stops = {
            { 0  , "#ff0000" },
            { 0.5, "#00ff00" },
            { 1  , "#0000ff" },
        }
    }
}

--DOC_NEWLINE

for k, stretch in ipairs { false, true } do
    --DOC_HIDE_START
    local r = (k-1) * 3 + 1

    l:add_widget_at(wibox.widget {
        markup = "<b>stretch_vertically = \"".. (stretch and "true" or "false") .."\"</b>",
        widget = wibox.widget.textbox,
    }, r, 1, 1, 2)
    --DOC_HIDE_END


    for _, gradient in ipairs(gradients) do
        for idx, height in ipairs { 10, 50, 100, 150 } do
            local w = wibox.widget {
                {
                    {
                        text   = "  Height: " .. height .. "  ",
                        valign = "center",
                        align  = "center",
                        widget = wibox.widget.textbox
                    },
                    bg                 = gradient,
                    stretch_vertically = stretch,
                    forced_height      = height,
                    fg                 = "#ffffff",
                    shape              = gears.shape.rounded_rect,
                    widget             = wibox.container.background
                },
                forced_height = 150,
                widget        = wibox.container.place
            }

            l:add_widget_at(w, r + _, idx, 1, 1) --DOC_HIDE
        end
    end
    --DOC_HIDE_END
end
--DOC_HIDE_START

parent:add(l)

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
