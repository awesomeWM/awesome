---------------------------------------------
-- Awesome theme which follows xrdb config --
--   by Yauhen Kirylau                    --
---------------------------------------------

local xresources = require("beautiful").xresources
local xrdb = xresources.get_current_theme()
local dpi = xresources.apply_dpi
local recolor_image = require("gears").color.recolor_image

-- inherit default theme
local theme = dofile("@AWESOME_THEMES_PATH@/default/theme.lua")
-- load vector assets' generators for this theme
local theme_assets = dofile("@AWESOME_THEMES_PATH@/xresources/assets.lua")

theme.font          = "sans 8"

theme.bg_normal     = xrdb.background
theme.bg_focus      = xrdb.color12
theme.bg_urgent     = xrdb.color9
theme.bg_minimize   = xrdb.color8
theme.bg_systray    = theme.bg_normal

theme.fg_normal     = xrdb.foreground
theme.fg_focus      = theme.bg_normal
theme.fg_urgent     = theme.bg_normal
theme.fg_minimize   = theme.bg_normal

theme.useless_gap   = dpi(5)
theme.border_width  = dpi(1)
theme.border_normal = xrdb.color0
theme.border_focus  = theme.bg_focus
theme.border_marked = xrdb.color10

-- There are other variable sets
-- overriding the default one when
-- defined, the sets are:
-- taglist_[bg|fg]_[focus|urgent|occupied|empty]
-- tasklist_[bg|fg]_[focus|urgent]
-- titlebar_[bg|fg]_[normal|focus]
-- tooltip_[font|opacity|fg_color|bg_color|border_width|border_color]
-- mouse_finder_[color|timeout|animate_timeout|radius|factor]
-- Example:
--theme.taglist_bg_focus = "#ff0000"

-- Variables set for theming the menu:
-- menu_[bg|fg]_[normal|focus]
-- menu_[border_color|border_width]
theme.menu_submenu_icon = "@AWESOME_THEMES_PATH@/default/submenu.png"
theme.menu_height = dpi(16)
theme.menu_width  = dpi(100)

-- You can add as many variables as
-- you wish and access them by using
-- beautiful.variable in your rc.lua
--theme.bg_widget = "#cc0000"

-- Recolor titlebar icons:
for _, titlebar_icon in ipairs({
    'titlebar_close_button_normal',

    'titlebar_ontop_button_normal_inactive',
    'titlebar_ontop_button_normal_active',

    'titlebar_sticky_button_normal_inactive',
    'titlebar_sticky_button_normal_active',

    'titlebar_floating_button_normal_inactive',
    'titlebar_floating_button_normal_active',

    'titlebar_maximized_button_normal_inactive',
    'titlebar_maximized_button_normal_active',
}) do
    theme[titlebar_icon] = recolor_image(theme[titlebar_icon], theme.fg_normal)
end
for _, titlebar_icon in ipairs({
    'titlebar_close_button_focus',

    'titlebar_ontop_button_focus_inactive',
    'titlebar_ontop_button_focus_active',

    'titlebar_sticky_button_focus_inactive',
    'titlebar_sticky_button_focus_active',

    'titlebar_floating_button_focus_inactive',
    'titlebar_floating_button_focus_active',

    'titlebar_maximized_button_focus_inactive',
    'titlebar_maximized_button_focus_active',
}) do
    theme[titlebar_icon] = recolor_image(theme[titlebar_icon], theme.fg_focus)
end

-- Recolor layout icons:
for _, layout_name in ipairs({
    'layout_fairh',
    'layout_fairv',
    'layout_floating',
    'layout_magnifier',
    'layout_max',
    'layout_fullscreen',
    'layout_tilebottom',
    'layout_tileleft',
    'layout_tile',
    'layout_tiletop',
    'layout_spiral',
    'layout_dwindle',
    'layout_cornernw',
    'layout_cornerne',
    'layout_cornersw',
    'layout_cornerse',
}) do
    theme[layout_name] = recolor_image(theme[layout_name], theme.fg_normal)
end

-- Define the icon theme for application icons. If not set then the icons 
-- from /usr/share/icons and /usr/share/icons/hicolor will be used.
theme.icon_theme = nil

-- Generate Awesome icon:
theme.awesome_icon = theme_assets.awesome_icon(
    theme.menu_height, theme.bg_focus, theme.fg_focus
)

-- Generate taglist squares:
local taglist_square_size = dpi(4)
theme.taglist_squares_sel = theme_assets.taglist_squares_sel(
    taglist_square_size, theme.fg_normal
)
theme.taglist_squares_unsel = theme_assets.taglist_squares_unsel(
    taglist_square_size, theme.fg_normal
)

-- Try to determine are we running light or dark colorscheme:
local bg_numberic_value = 0;
for s in theme.bg_normal:gmatch("[a-fA-F0-9][a-fA-F0-9]") do
    bg_numberic_value = bg_numberic_value + tonumber("0x"..s);
end
local is_dark_bg = (bg_numberic_value < 383)

-- Generate wallpaper:
local wallpaper_bg = xrdb.color8
local wallpaper_fg = xrdb.color7
local wallpaper_alt_fg = xrdb.color12
if not is_dark_bg then
    wallpaper_bg, wallpaper_fg = wallpaper_fg, wallpaper_bg
end
theme.wallpaper = theme_assets.wallpaper(
    wallpaper_bg, wallpaper_fg, wallpaper_alt_fg
)

return theme
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
