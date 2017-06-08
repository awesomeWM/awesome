-------------------------------
--    "Sky" awesome theme    --
--  By Andrei "Garoth" Thorp --
-------------------------------
-- If you want SVGs and extras, get them from garoth.com/awesome/sky-theme

local theme_assets = require("beautiful.theme_assets")
local xresources = require("beautiful.xresources")
local dpi = xresources.apply_dpi
local themes_path = require("gears.filesystem").get_themes_dir()


-- BASICS
local theme = {}
theme.font          = "sans 8"

theme.bg_focus      = "#e2eeea"
theme.bg_normal     = "#729fcf"
theme.bg_urgent     = "#fce94f"
theme.bg_minimize   = "#0067ce"
theme.bg_systray    = theme.bg_normal

theme.fg_normal     = "#2e3436"
theme.fg_focus      = "#2e3436"
theme.fg_urgent     = "#2e3436"
theme.fg_minimize   = "#2e3436"

theme.useless_gap   = dpi(0)
theme.border_width  = dpi(2)
theme.border_normal = "#dae3e0"
theme.border_focus  = "#729fcf"
theme.border_marked = "#eeeeec"

-- IMAGES
theme.layout_fairh           = themes_path .. "sky/layouts/fairh.png"
theme.layout_fairv           = themes_path .. "sky/layouts/fairv.png"
theme.layout_floating        = themes_path .. "sky/layouts/floating.png"
theme.layout_magnifier       = themes_path .. "sky/layouts/magnifier.png"
theme.layout_max             = themes_path .. "sky/layouts/max.png"
theme.layout_fullscreen      = themes_path .. "sky/layouts/fullscreen.png"
theme.layout_tilebottom      = themes_path .. "sky/layouts/tilebottom.png"
theme.layout_tileleft        = themes_path .. "sky/layouts/tileleft.png"
theme.layout_tile            = themes_path .. "sky/layouts/tile.png"
theme.layout_tiletop         = themes_path .. "sky/layouts/tiletop.png"
theme.layout_spiral          = themes_path .. "sky/layouts/spiral.png"
theme.layout_dwindle         = themes_path .. "sky/layouts/dwindle.png"
theme.layout_cornernw        = themes_path .. "sky/layouts/cornernw.png"
theme.layout_cornerne        = themes_path .. "sky/layouts/cornerne.png"
theme.layout_cornersw        = themes_path .. "sky/layouts/cornersw.png"
theme.layout_cornerse        = themes_path .. "sky/layouts/cornerse.png"

theme.awesome_icon           = themes_path .. "sky/awesome-icon.png"

-- from default for now...
theme.menu_submenu_icon     = themes_path .. "default/submenu.png"

-- Generate taglist squares:
local taglist_square_size = dpi(4)
theme.taglist_squares_sel = theme_assets.taglist_squares_sel(
    taglist_square_size, theme.fg_normal
)
theme.taglist_squares_unsel = theme_assets.taglist_squares_unsel(
    taglist_square_size, theme.fg_normal
)

-- MISC
theme.wallpaper             = themes_path .. "sky/sky-background.png"
theme.taglist_squares       = "true"
theme.titlebar_close_button = "true"
theme.menu_height = dpi(15)
theme.menu_width  = dpi(100)

-- Define the image to load
theme.titlebar_close_button_normal = themes_path .. "default/titlebar/close_normal.png"
theme.titlebar_close_button_focus = themes_path .. "default/titlebar/close_focus.png"

theme.titlebar_minimize_button_normal = themes_path .. "default/titlebar/minimize_normal.png"
theme.titlebar_minimize_button_focus  = themes_path .. "default/titlebar/minimize_focus.png"

theme.titlebar_ontop_button_normal_inactive = themes_path .. "default/titlebar/ontop_normal_inactive.png"
theme.titlebar_ontop_button_focus_inactive = themes_path .. "default/titlebar/ontop_focus_inactive.png"
theme.titlebar_ontop_button_normal_active = themes_path .. "default/titlebar/ontop_normal_active.png"
theme.titlebar_ontop_button_focus_active = themes_path .. "default/titlebar/ontop_focus_active.png"

theme.titlebar_sticky_button_normal_inactive = themes_path .. "default/titlebar/sticky_normal_inactive.png"
theme.titlebar_sticky_button_focus_inactive = themes_path .. "default/titlebar/sticky_focus_inactive.png"
theme.titlebar_sticky_button_normal_active = themes_path .. "default/titlebar/sticky_normal_active.png"
theme.titlebar_sticky_button_focus_active = themes_path .. "default/titlebar/sticky_focus_active.png"

theme.titlebar_floating_button_normal_inactive = themes_path .. "default/titlebar/floating_normal_inactive.png"
theme.titlebar_floating_button_focus_inactive = themes_path .. "default/titlebar/floating_focus_inactive.png"
theme.titlebar_floating_button_normal_active = themes_path .. "default/titlebar/floating_normal_active.png"
theme.titlebar_floating_button_focus_active = themes_path .. "default/titlebar/floating_focus_active.png"

theme.titlebar_maximized_button_normal_inactive = themes_path .. "default/titlebar/maximized_normal_inactive.png"
theme.titlebar_maximized_button_focus_inactive = themes_path .. "default/titlebar/maximized_focus_inactive.png"
theme.titlebar_maximized_button_normal_active = themes_path .. "default/titlebar/maximized_normal_active.png"
theme.titlebar_maximized_button_focus_active = themes_path .. "default/titlebar/maximized_focus_active.png"

return theme

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
