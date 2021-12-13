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

-- Use a cairo pattern for the foreground to allow improve the
-- reliability of the find&replace documentation script. This
-- will be post-processed to ensure the foreground color is
-- inherited from the browser rather than hardcoded. In turn,
-- this allows the accessibility mode to work and to define the
-- color using our CSS template, which allows for light/dark
-- themes to be implemented with a single image.
local main_fg = cairo.Pattern.create_rgba(0.005, 0, 0.005, 1)

-- Default theme for the documentation examples
local module = {
    fg_normal    = main_fg,
    bg_normal    = "#6181FF7D",
    bg_focus     = "#AA00FF7D",
    bg_highlight = "#AA00FF7D",
    border_color = "#6181FF"  ,
    border_width = 1.5        ,

    prompt_bg_cursor = "#00FF7D",

    -- Fake resources handling
    xresources = require("beautiful.xresources"),

    awesome_icon = logo()
}

module.graph_bg = module.bg_normal
module.graph_fg = module.bg_highlight

module.progressbar_bg = module.bg_normal
module.progressbar_fg = module.bg_highlight

local f = Pango.FontDescription.from_string("sans 8")

function module.get_font(font)
    if font then
        return Pango.FontDescription.from_string(font)
    end

    return f
end

function module.get_font_height()
    return 9
end

------------------------------------------------------------------
-- Import the titlebar and layout assets from the default theme --
------------------------------------------------------------------

-- It's fine as long as gears doesn't depend on CAPI and $AWESOME_THEMES_PATH is set.
local themes_path = require("gears.filesystem").get_themes_dir()

-- Define the image to load
module.titlebar_close_button_normal = themes_path.."default/titlebar/close_normal.png"
module.titlebar_close_button_focus  = themes_path.."default/titlebar/close_focus.png"

module.titlebar_minimize_button_normal = themes_path.."default/titlebar/minimize_normal.png"
module.titlebar_minimize_button_focus  = themes_path.."default/titlebar/minimize_focus.png"

module.titlebar_ontop_button_normal_inactive = themes_path.."default/titlebar/ontop_normal_inactive.png"
module.titlebar_ontop_button_focus_inactive  = themes_path.."default/titlebar/ontop_focus_inactive.png"
module.titlebar_ontop_button_normal_active = themes_path.."default/titlebar/ontop_normal_active.png"
module.titlebar_ontop_button_focus_active  = themes_path.."default/titlebar/ontop_focus_active.png"

module.titlebar_sticky_button_normal_inactive = themes_path.."default/titlebar/sticky_normal_inactive.png"
module.titlebar_sticky_button_focus_inactive  = themes_path.."default/titlebar/sticky_focus_inactive.png"
module.titlebar_sticky_button_normal_active = themes_path.."default/titlebar/sticky_normal_active.png"
module.titlebar_sticky_button_focus_active  = themes_path.."default/titlebar/sticky_focus_active.png"

module.titlebar_floating_button_normal_inactive = themes_path.."default/titlebar/floating_normal_inactive.png"
module.titlebar_floating_button_focus_inactive  = themes_path.."default/titlebar/floating_focus_inactive.png"
module.titlebar_floating_button_normal_active = themes_path.."default/titlebar/floating_normal_active.png"
module.titlebar_floating_button_focus_active  = themes_path.."default/titlebar/floating_focus_active.png"

module.titlebar_maximized_button_normal_inactive = themes_path.."default/titlebar/maximized_normal_inactive.png"
module.titlebar_maximized_button_focus_inactive  = themes_path.."default/titlebar/maximized_focus_inactive.png"
module.titlebar_maximized_button_normal_active = themes_path.."default/titlebar/maximized_normal_active.png"
module.titlebar_maximized_button_focus_active  = themes_path.."default/titlebar/maximized_focus_active.png"

module.wallpaper = themes_path.."default/background.png"

-- You can use your own layout icons like this:
module.layout_fairh = themes_path.."default/layouts/fairhw.png"
module.layout_fairv = themes_path.."default/layouts/fairvw.png"
module.layout_floating  = themes_path.."default/layouts/floatingw.png"
module.layout_magnifier = themes_path.."default/layouts/magnifierw.png"
module.layout_max = themes_path.."default/layouts/maxw.png"
module.layout_fullscreen = themes_path.."default/layouts/fullscreenw.png"
module.layout_tilebottom = themes_path.."default/layouts/tilebottomw.png"
module.layout_tileleft   = themes_path.."default/layouts/tileleftw.png"
module.layout_tile = themes_path.."default/layouts/tilew.png"
module.layout_tiletop = themes_path.."default/layouts/tiletopw.png"
module.layout_spiral  = themes_path.."default/layouts/spiralw.png"
module.layout_dwindle = themes_path.."default/layouts/dwindlew.png"
module.layout_cornernw = themes_path.."default/layouts/cornernww.png"
module.layout_cornerne = themes_path.."default/layouts/cornernew.png"
module.layout_cornersw = themes_path.."default/layouts/cornersww.png"
module.layout_cornerse = themes_path.."default/layouts/cornersew.png"

-- Taglist
module.taglist_bg_focus  = module.bg_highlight
module.taglist_bg_used   = module.bg_normal
module.taglist_bg_normal = module.bg_normal


function module.get()
    return module
end

return module

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
