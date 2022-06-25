------------------------------------------
-- Aesthetic Night awesomewm theme      --
-- Created by https://github.com/rxyhn  --
------------------------------------------

local theme_assets = require("beautiful.theme_assets")
local xresources = require("beautiful.xresources")
local rnotification = require("ruled.notification")
local dpi = xresources.apply_dpi

local gfs = require("gears.filesystem")
local themes_path = gfs.get_themes_dir()

-- inherit default theme
local theme = dofile(themes_path .. "default/theme.lua")
-- load vector assets' generators for this theme

theme.font = "sans 8"

theme.bg_normal = "#061115"
theme.bg_focus = "#0d181c"
theme.bg_urgent = "#df5b61"
theme.bg_minimize = "#000a0e"
theme.bg_systray = theme.bg_normal

theme.fg_normal = "#d9d7d6"
theme.fg_focus = "#6791c9"
theme.fg_urgent = "#d9d7d6"
theme.fg_minimize = "#d9d7d6"

theme.useless_gap = dpi(2)
theme.border_width = dpi(2)
theme.border_color_normal = "#061115"
theme.border_color_active = "#000a0e"
theme.border_color_marked = "#000a0e"

-- Generate taglist squares:
--local taglist_square_size = dpi(4)
--theme.taglist_squares_sel = theme_assets.taglist_squares_sel(taglist_square_size, theme.fg_normal)
--theme.taglist_squares_unsel = theme_assets.taglist_squares_unsel(taglist_square_size, theme.fg_normal)
-- Or disable them:
theme.taglist_squares_sel = nil
theme.taglist_squares_unsel = nil

-- Taglist
theme.taglist_fg_focus = theme.bg_normal
theme.taglist_bg_focus = theme.fg_focus

-- Variables set for theming the menu:
theme.menu_submenu_icon = nil
theme.menu_submenu = "▸ "
theme.menu_height = dpi(15)
theme.menu_width = dpi(100)

-- Notification
theme.notification_border_color = theme.border_color_active

-- Wallpaper
theme.wallpaper = themes_path .. "night/background.png"

-- Recolor Layout icons:
theme = theme_assets.recolor_layout(theme, theme.fg_normal)

-- Generate Awesome icon:
theme.awesome_icon = theme_assets.awesome_icon(theme.menu_height, theme.fg_focus, theme.bg_minimize)

-- Set different colors for urgent notifications.
rnotification.connect_signal("request::rules", function()
	rnotification.append_rule({
		rule = { urgency = "critical" },
		properties = { bg = "#df5b61", fg = "#d9d7d6" },
	})
end)

-- Hotkeys Pop Up
theme.hotkeys_bg = theme.bg_normal
theme.hotkeys_fg = theme.fg_normal
theme.hotkeys_modifiers_fg = theme.fg_normal
theme.hotkeys_border_color = theme.border_color_active
theme.hotkeys_group_margin = dpi(50)

return theme

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
