-------------------------------
--    "Sky" awesome theme    --
--  By Andrei "Garoth" Thorp --
-------------------------------
-- If you want SVGs and extras, get them from garoth.com/awesome/sky-theme

local theme_assets = require("beautiful.theme_assets")
local xresources = require("beautiful.xresources")
local dpi = xresources.apply_dpi


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

theme.useless_gap   = 0
theme.border_width  = dpi(2)
theme.border_normal = "#dae3e0"
theme.border_focus  = "#729fcf"
theme.border_marked = "#eeeeec"

-- IMAGES
theme.layout_fairh           = "@AWESOME_THEMES_PATH@/sky/layouts/fairh.png"
theme.layout_fairv           = "@AWESOME_THEMES_PATH@/sky/layouts/fairv.png"
theme.layout_floating        = "@AWESOME_THEMES_PATH@/sky/layouts/floating.png"
theme.layout_magnifier       = "@AWESOME_THEMES_PATH@/sky/layouts/magnifier.png"
theme.layout_max             = "@AWESOME_THEMES_PATH@/sky/layouts/max.png"
theme.layout_fullscreen      = "@AWESOME_THEMES_PATH@/sky/layouts/fullscreen.png"
theme.layout_tilebottom      = "@AWESOME_THEMES_PATH@/sky/layouts/tilebottom.png"
theme.layout_tileleft        = "@AWESOME_THEMES_PATH@/sky/layouts/tileleft.png"
theme.layout_tile            = "@AWESOME_THEMES_PATH@/sky/layouts/tile.png"
theme.layout_tiletop         = "@AWESOME_THEMES_PATH@/sky/layouts/tiletop.png"
theme.layout_spiral          = "@AWESOME_THEMES_PATH@/sky/layouts/spiral.png"
theme.layout_dwindle         = "@AWESOME_THEMES_PATH@/sky/layouts/dwindle.png"
theme.layout_cornernw        = "@AWESOME_THEMES_PATH@/sky/layouts/cornernw.png"
theme.layout_cornerne        = "@AWESOME_THEMES_PATH@/sky/layouts/cornerne.png"
theme.layout_cornersw        = "@AWESOME_THEMES_PATH@/sky/layouts/cornersw.png"
theme.layout_cornerse        = "@AWESOME_THEMES_PATH@/sky/layouts/cornerse.png"

theme.awesome_icon           = "@AWESOME_THEMES_PATH@/sky/awesome-icon.png"

-- from default for now...
theme.menu_submenu_icon     = "@AWESOME_THEMES_PATH@/default/submenu.png"

-- Generate taglist squares:
local taglist_square_size = dpi(4)
theme.taglist_squares_sel = theme_assets.taglist_squares_sel(
    taglist_square_size, theme.fg_normal
)
theme.taglist_squares_unsel = theme_assets.taglist_squares_unsel(
    taglist_square_size, theme.fg_normal
)

-- MISC
theme.wallpaper             = "@AWESOME_THEMES_PATH@/sky/sky-background.png"
theme.taglist_squares       = "true"
theme.titlebar_close_button = "true"
theme.menu_height = dpi(15)
theme.menu_width  = dpi(100)

-- Define the image to load
theme.titlebar_close_button_normal = "@AWESOME_THEMES_PATH@/default/titlebar/close_normal.png"
theme.titlebar_close_button_focus = "@AWESOME_THEMES_PATH@/default/titlebar/close_focus.png"

theme.titlebar_minimize_button_normal = "@AWESOME_THEMES_PATH@/default/titlebar/minimize_normal.png"
theme.titlebar_minimize_button_focus  = "@AWESOME_THEMES_PATH@/default/titlebar/minimize_focus.png"

theme.titlebar_ontop_button_normal_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/ontop_normal_inactive.png"
theme.titlebar_ontop_button_focus_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/ontop_focus_inactive.png"
theme.titlebar_ontop_button_normal_active = "@AWESOME_THEMES_PATH@/default/titlebar/ontop_normal_active.png"
theme.titlebar_ontop_button_focus_active = "@AWESOME_THEMES_PATH@/default/titlebar/ontop_focus_active.png"

theme.titlebar_sticky_button_normal_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/sticky_normal_inactive.png"
theme.titlebar_sticky_button_focus_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/sticky_focus_inactive.png"
theme.titlebar_sticky_button_normal_active = "@AWESOME_THEMES_PATH@/default/titlebar/sticky_normal_active.png"
theme.titlebar_sticky_button_focus_active = "@AWESOME_THEMES_PATH@/default/titlebar/sticky_focus_active.png"

theme.titlebar_floating_button_normal_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/floating_normal_inactive.png"
theme.titlebar_floating_button_focus_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/floating_focus_inactive.png"
theme.titlebar_floating_button_normal_active = "@AWESOME_THEMES_PATH@/default/titlebar/floating_normal_active.png"
theme.titlebar_floating_button_focus_active = "@AWESOME_THEMES_PATH@/default/titlebar/floating_focus_active.png"

theme.titlebar_maximized_button_normal_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/maximized_normal_inactive.png"
theme.titlebar_maximized_button_focus_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/maximized_focus_inactive.png"
theme.titlebar_maximized_button_normal_active = "@AWESOME_THEMES_PATH@/default/titlebar/maximized_normal_active.png"
theme.titlebar_maximized_button_focus_active = "@AWESOME_THEMES_PATH@/default/titlebar/maximized_focus_active.png"

return theme

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
