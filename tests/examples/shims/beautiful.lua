local lgi = require("lgi")
local Pango = lgi.Pango
local cairo = lgi.cairo

-- A simple Awesome logo
local function logo()
    local img = cairo.ImageSurface.create(cairo.Format.ARGB32, 22, 22)
    local cr = cairo.Context(img)

    -- Awesome default #555555
    cr:set_source_rgb(0.21568627451, 0.21568627451, 0.21568627451)
    cr:paint()

    cr:set_source_rgb(1,1,1)

    cr:rectangle(0, 7, 15, 1)
    cr:fill()

    cr:rectangle(15, 15, 1, 7)
    cr:fill()

    cr:rectangle(8, 15, 7, 1)
    cr:fill()

    return img
end

-- Default theme for the documentation examples
local module = {
    fg_normal    = "#000000"  ,
    bg_normal    = "#6181FF7D",
    bg_highlight = "#AA00FF7D",
    border_color = "#6181FF"  ,
    border_width = 1.5        ,

    -- Fake resources handling
    xresources = require("beautiful.xresources"),

    awesome_icon = logo()
}

module.graph_bg = module.bg_normal
module.graph_fg = module.bg_highlight

module.progressbar_bg = module.bg_normal
module.progressbar_fg = module.bg_highlight

local f = Pango.FontDescription.from_string("sans 8")

function module.get_font()
    return f
end

function module.get()
    return module
end

return module

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
