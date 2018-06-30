----------------------------------------------
-- Awesome theme which follows GTK+ 3 theme --
--   by Yauhen Kirylau                      --
----------------------------------------------

local theme_assets = require("beautiful.theme_assets")
local dpi = require("beautiful.xresources").apply_dpi
local gfs = require("gears.filesystem")
local themes_path = gfs.get_themes_dir()
local gears_shape = require("gears.shape")
local wibox = require("wibox")
local awful_widget_clienticon = require("awful.widget.clienticon")
local gtk = require("beautiful.gtk")


-- Helper functions for modifying hex colors:
--
local hex_color_match = "[a-fA-F0-9][a-fA-F0-9]"
local function darker(color_value, darker_n)
    local result = "#"
    local channel_counter = 1
    for s in color_value:gmatch(hex_color_match) do
        local bg_numeric_value = tonumber("0x"..s)
        if channel_counter <= 3 then
            bg_numeric_value = bg_numeric_value - darker_n
        end
        if bg_numeric_value < 0 then bg_numeric_value = 0 end
        if bg_numeric_value > 255 then bg_numeric_value = 255 end
        result = result .. string.format("%02x", bg_numeric_value)
        channel_counter = channel_counter + 1
    end
    return result
end
local function is_dark(color_value)
    local bg_numeric_value = 0;
    local channel_counter = 1
    for s in color_value:gmatch(hex_color_match) do
        bg_numeric_value = bg_numeric_value + tonumber("0x"..s);
        if channel_counter == 3 then
            break
        end
        channel_counter = channel_counter + 1
    end
    local is_dark_bg = (bg_numeric_value < 383)
    return is_dark_bg
end
local function mix(color1, color2, ratio)
    ratio = ratio or 0.5
    local result = "#"
    local channels1 = color1:gmatch(hex_color_match)
    local channels2 = color2:gmatch(hex_color_match)
    for _ = 1,3 do
        local bg_numeric_value = math.ceil(
          tonumber("0x"..channels1())*ratio +
          tonumber("0x"..channels2())*(1-ratio)
        )
        if bg_numeric_value < 0 then bg_numeric_value = 0 end
        if bg_numeric_value > 255 then bg_numeric_value = 255 end
        result = result .. string.format("%02x", bg_numeric_value)
    end
    return result
end
local function reduce_contrast(color, ratio)
    ratio = ratio or 50
    return darker(color, is_dark(color) and -ratio or ratio)
end

local function choose_contrast_color(reference, candidate1, candidate2)  -- luacheck: no unused
    if is_dark(reference) then
        if not is_dark(candidate1) then
            return candidate1
        else
            return candidate2
        end
    else
        if is_dark(candidate1) then
            return candidate1
        else
            return candidate2
        end
    end
end


-- inherit xresources theme:
local theme = dofile(themes_path.."xresources/theme.lua")
-- load and prepare for use gtk theme:
theme.gtk = gtk.get_theme_variables()
if not theme.gtk then
    local gears_debug = require("gears.debug")
    gears_debug.print_warning("Can't load GTK+3 theme. Using 'xresources' theme as a fallback.")
    return theme
end
theme.gtk.button_border_radius = dpi(theme.gtk.button_border_radius or 0)
theme.gtk.button_border_width = dpi(theme.gtk.button_border_width or 1)
theme.gtk.bold_font = theme.gtk.font_family .. ' Bold ' .. theme.gtk.font_size
theme.gtk.menubar_border_color = mix(
    theme.gtk.menubar_bg_color,
    theme.gtk.menubar_fg_color,
    0.7
)


theme.font          = theme.gtk.font_family .. ' ' .. theme.gtk.font_size

theme.bg_normal     = theme.gtk.bg_color
theme.fg_normal     = theme.gtk.fg_color

theme.wibar_bg      = theme.gtk.menubar_bg_color
theme.wibar_fg      = theme.gtk.menubar_fg_color

theme.bg_focus      = theme.gtk.selected_bg_color
theme.fg_focus      = theme.gtk.selected_fg_color

theme.bg_urgent     = theme.gtk.error_bg_color
theme.fg_urgent     = theme.gtk.error_fg_color

theme.bg_minimize   = mix(theme.wibar_fg, theme.wibar_bg, 0.3)
theme.fg_minimize   = mix(theme.wibar_fg, theme.wibar_bg, 0.9)

theme.bg_systray    = theme.wibar_bg

theme.border_normal = theme.gtk.wm_border_unfocused_color
theme.border_focus  = theme.gtk.wm_border_focused_color
theme.border_marked = theme.gtk.success_color

theme.border_width  = dpi(theme.gtk.button_border_width or 1)
theme.border_radius = theme.gtk.button_border_radius

theme.useless_gap   = dpi(3)

local rounded_rect_shape = function(cr,w,h)
    gears_shape.rounded_rect(
        cr, w, h, theme.border_radius
    )
end

-- There are other variable sets
-- overriding the default one when
-- defined, the sets are:
-- taglist_[bg|fg|shape|shape_border_color|shape_border_width]_[focus|urgent|occupied|empty|volatile]
-- tasklist_[bg|fg|shape|shape_border_color|shape_border_width]_[focus|urgent|minimized]
-- titlebar_[bg|fg]_[normal|focus]
-- tooltip_[font|opacity|fg_color|bg_color|border_width|border_color]
-- mouse_finder_[color|timeout|animate_timeout|radius|factor]

theme.tasklist_fg_normal = theme.wibar_fg
theme.tasklist_bg_normal = theme.wibar_bg
theme.tasklist_fg_focus = theme.tasklist_fg_normal
theme.tasklist_bg_focus = theme.tasklist_bg_normal

theme.tasklist_font_focus = theme.gtk.bold_font

theme.tasklist_shape_minimized = rounded_rect_shape
theme.tasklist_shape_border_color_minimized = mix(
    theme.bg_minimize,
    theme.fg_minimize,
    0.85
)
theme.tasklist_shape_border_width_minimized = theme.gtk.button_border_width

theme.tasklist_spacing = theme.gtk.button_border_width

--[[ Advanced taglist and tasklist styling: {{{

--- In order to get taglist and tasklist to follow GTK theme you need to
-- modify your rc.lua in the following way:

diff --git a/rc.lua b/rc.lua
index 231a2f68c..533a859d2 100644
--- a/rc.lua
+++ b/rc.lua
@@ -217,24 +217,12 @@ awful.screen.connect_for_each_screen(function(s)
         filter  = awful.widget.taglist.filter.all,
         buttons = taglist_buttons
     }
+    -- and apply shape to it
+    if beautiful.taglist_shape_container then
+        local background_shape_wrapper = wibox.container.background(s.mytaglist)
+        background_shape_wrapper._do_taglist_update_now = s.mytaglist._do_taglist_update_now
+        background_shape_wrapper._do_taglist_update = s.mytaglist._do_taglist_update
+        background_shape_wrapper.shape = beautiful.taglist_shape_container
+        background_shape_wrapper.shape_clip = beautiful.taglist_shape_clip_container
+        background_shape_wrapper.shape_border_width = beautiful.taglist_shape_border_width_container
+        background_shape_wrapper.shape_border_color = beautiful.taglist_shape_border_color_container
+        s.mytaglist = background_shape_wrapper
+    end

     -- Create a tasklist widget
     s.mytasklist = awful.widget.tasklist {
         screen  = s,
         filter  = awful.widget.tasklist.filter.currenttags,
+        buttons = tasklist_buttons,
+        widget_template = beautiful.tasklist_widget_template
-        buttons = tasklist_buttons
     }

--]]
theme.tasklist_widget_template = {
    {
        {
            {
                {
                    id     = 'clienticon',
                    widget = awful_widget_clienticon,
                },
                margins = dpi(4),
                widget  = wibox.container.margin,
            },
            {
                id     = 'text_role',
                widget = wibox.widget.textbox,
            },
            layout = wibox.layout.fixed.horizontal,
        },
        left  = dpi(2),
        right = dpi(4),
        widget = wibox.container.margin
    },
    id     = 'background_role',
    widget = wibox.container.background,
    create_callback = function(self, c)
        self:get_children_by_id('clienticon')[1].client = c
    end,
}

theme.taglist_shape_container = rounded_rect_shape
theme.taglist_shape_clip_container = true
theme.taglist_shape_border_width_container = theme.gtk.button_border_width * 2
theme.taglist_shape_border_color_container = theme.gtk.header_button_border_color
-- }}}

theme.taglist_bg_occupied = theme.gtk.header_button_bg_color
theme.taglist_fg_occupied = theme.gtk.header_button_fg_color

theme.taglist_bg_empty = mix(
    theme.gtk.menubar_bg_color,
    theme.gtk.header_button_bg_color,
    0.3
)
theme.taglist_fg_empty = mix(
    theme.gtk.menubar_bg_color,
    theme.gtk.header_button_fg_color
)

theme.titlebar_font_normal = theme.gtk.bold_font
theme.titlebar_bg_normal = theme.gtk.wm_border_unfocused_color
theme.titlebar_fg_normal = theme.gtk.wm_title_unfocused_color
--theme.titlebar_fg_normal = choose_contrast_color(
    --theme.titlebar_bg_normal,
    --theme.gtk.menubar_fg_color,
    --theme.gtk.menubar_bg_color
--)

theme.titlebar_font_focus = theme.gtk.bold_font
theme.titlebar_bg_focus = theme.gtk.wm_border_focused_color
theme.titlebar_fg_focus = theme.gtk.wm_title_focused_color
--theme.titlebar_fg_focus = choose_contrast_color(
    --theme.titlebar_bg_focus,
    --theme.gtk.menubar_fg_color,
    --theme.gtk.menubar_bg_color
--)

theme.tooltip_fg = theme.gtk.tooltip_fg_color
theme.tooltip_bg = theme.gtk.tooltip_bg_color

-- Variables set for theming the menu:
-- menu_[bg|fg]_[normal|focus]
-- menu_[border_color|border_width]

theme.menu_border_width = theme.gtk.button_border_width
theme.menu_border_color = theme.gtk.menubar_border_color
theme.menu_bg_normal = theme.gtk.menubar_bg_color
theme.menu_fg_normal = theme.gtk.menubar_fg_color

-- @TODO: get from gtk menu height
theme.menu_height = dpi(24)
theme.menu_width  = dpi(150)
theme.menu_submenu_icon = nil
theme.menu_submenu = "▸ "

-- You can add as many variables as
-- you wish and access them by using
-- beautiful.variable in your rc.lua
--theme.bg_widget = "#cc0000"


-- Recolor Layout icons:
theme = theme_assets.recolor_layout(theme, theme.wibar_fg)

-- Recolor titlebar icons:
--
theme = theme_assets.recolor_titlebar(
    theme, theme.titlebar_fg_normal, "normal"
)
theme = theme_assets.recolor_titlebar(
    theme, reduce_contrast(theme.titlebar_fg_normal, 50), "normal", "hover"
)
theme = theme_assets.recolor_titlebar(
    theme, theme.gtk.error_bg_color, "normal", "press"
)
theme = theme_assets.recolor_titlebar(
    theme, theme.titlebar_fg_focus, "focus"
)
theme = theme_assets.recolor_titlebar(
    theme, reduce_contrast(theme.titlebar_fg_focus, 50), "focus", "hover"
)
theme = theme_assets.recolor_titlebar(
    theme, theme.gtk.error_bg_color, "focus", "press"
)

-- Define the icon theme for application icons. If not set then the icons
-- from /usr/share/icons and /usr/share/icons/hicolor will be used.
theme.icon_theme = nil

-- Generate Awesome icon:
theme.awesome_icon = theme_assets.awesome_icon(
    theme.menu_height, mix(theme.bg_focus, theme.fg_normal), theme.wibar_bg
)

-- Generate taglist squares:
--local taglist_square_size = dpi(4)
--theme.taglist_squares_sel = theme_assets.taglist_squares_sel(
    --taglist_square_size, theme.gtk.header_button_border_color
--)
--theme.taglist_squares_unsel = theme_assets.taglist_squares_unsel(
    --taglist_square_size, theme.gtk.header_button_border_color
--)
-- Or disable them:
theme.taglist_squares_sel = nil
theme.taglist_squares_unsel = nil

-- Generate wallpaper:
local wallpaper_bg = theme.gtk.base_color
local wallpaper_fg = theme.gtk.bg_color
local wallpaper_alt_fg = theme.gtk.selected_bg_color
if not is_dark(theme.bg_normal) then
    wallpaper_bg, wallpaper_fg = wallpaper_fg, wallpaper_bg
end
wallpaper_bg = reduce_contrast(wallpaper_bg, 50)
wallpaper_fg = reduce_contrast(wallpaper_fg, 30)
wallpaper_fg = mix(wallpaper_fg, wallpaper_bg, 0.4)
wallpaper_alt_fg = mix(wallpaper_alt_fg, wallpaper_fg, 0.4)
theme.wallpaper = function(s)
    return theme_assets.wallpaper(wallpaper_bg, wallpaper_fg, wallpaper_alt_fg, s)
end

return theme

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80:foldmethod=marker
