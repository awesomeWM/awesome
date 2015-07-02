---------------------------------------------
-- Awesome theme which follows xrdb config --
--   by Yauhen Kirylau                    --
---------------------------------------------

local xresources = require("beautiful").xresources
local xrdb = xresources.get_current_theme()
local dpi = xresources.apply_dpi

local theme = {}

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

-- Define the image to load
theme.titlebar_close_button_normal = "@AWESOME_THEMES_PATH@/default/titlebar/close_normal.png"
theme.titlebar_close_button_focus  = "@AWESOME_THEMES_PATH@/default/titlebar/close_focus.png"

theme.titlebar_ontop_button_normal_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/ontop_normal_inactive.png"
theme.titlebar_ontop_button_focus_inactive  = "@AWESOME_THEMES_PATH@/default/titlebar/ontop_focus_inactive.png"
theme.titlebar_ontop_button_normal_active = "@AWESOME_THEMES_PATH@/default/titlebar/ontop_normal_active.png"
theme.titlebar_ontop_button_focus_active  = "@AWESOME_THEMES_PATH@/default/titlebar/ontop_focus_active.png"

theme.titlebar_sticky_button_normal_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/sticky_normal_inactive.png"
theme.titlebar_sticky_button_focus_inactive  = "@AWESOME_THEMES_PATH@/default/titlebar/sticky_focus_inactive.png"
theme.titlebar_sticky_button_normal_active = "@AWESOME_THEMES_PATH@/default/titlebar/sticky_normal_active.png"
theme.titlebar_sticky_button_focus_active  = "@AWESOME_THEMES_PATH@/default/titlebar/sticky_focus_active.png"

theme.titlebar_floating_button_normal_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/floating_normal_inactive.png"
theme.titlebar_floating_button_focus_inactive  = "@AWESOME_THEMES_PATH@/default/titlebar/floating_focus_inactive.png"
theme.titlebar_floating_button_normal_active = "@AWESOME_THEMES_PATH@/default/titlebar/floating_normal_active.png"
theme.titlebar_floating_button_focus_active  = "@AWESOME_THEMES_PATH@/default/titlebar/floating_focus_active.png"

theme.titlebar_maximized_button_normal_inactive = "@AWESOME_THEMES_PATH@/default/titlebar/maximized_normal_inactive.png"
theme.titlebar_maximized_button_focus_inactive  = "@AWESOME_THEMES_PATH@/default/titlebar/maximized_focus_inactive.png"
theme.titlebar_maximized_button_normal_active = "@AWESOME_THEMES_PATH@/default/titlebar/maximized_normal_active.png"
theme.titlebar_maximized_button_focus_active  = "@AWESOME_THEMES_PATH@/default/titlebar/maximized_focus_active.png"

-- Use 'w' postfix for dark background:
local bg_numberic_value = 0;
for s in theme.bg_normal:gmatch("[a-fA-F0-9][a-fA-F0-9]") do
    bg_numberic_value = bg_numberic_value + tonumber("0x"..s);
end
local is_dark_bg = (bg_numberic_value < 383)
local pf = is_dark_bg and 'w' or ''
-- You can use your own layout icons like this:
theme.layout_fairh = "@AWESOME_THEMES_PATH@/default/layouts/fairh" .. pf .. ".png"
theme.layout_fairv = "@AWESOME_THEMES_PATH@/default/layouts/fairv" .. pf .. ".png"
theme.layout_floating  = "@AWESOME_THEMES_PATH@/default/layouts/floating" .. pf .. ".png"
theme.layout_magnifier = "@AWESOME_THEMES_PATH@/default/layouts/magnifier" .. pf .. ".png"
theme.layout_max = "@AWESOME_THEMES_PATH@/default/layouts/max" .. pf .. ".png"
theme.layout_fullscreen = "@AWESOME_THEMES_PATH@/default/layouts/fullscreen" .. pf .. ".png"
theme.layout_tilebottom = "@AWESOME_THEMES_PATH@/default/layouts/tilebottom" .. pf .. ".png"
theme.layout_tileleft   = "@AWESOME_THEMES_PATH@/default/layouts/tileleft" .. pf .. ".png"
theme.layout_tile = "@AWESOME_THEMES_PATH@/default/layouts/tile" .. pf .. ".png"
theme.layout_tiletop = "@AWESOME_THEMES_PATH@/default/layouts/tiletop" .. pf .. ".png"
theme.layout_spiral  = "@AWESOME_THEMES_PATH@/default/layouts/spiral" .. pf .. ".png"
theme.layout_dwindle = "@AWESOME_THEMES_PATH@/default/layouts/dwindle" .. pf .. ".png"


-- Define the icon theme for application icons. If not set then the icons 
-- from /usr/share/icons and /usr/share/icons/hicolor will be used.
theme.icon_theme = nil


--------------------------------------------------
-- Generate vector assets using current colors: --
--------------------------------------------------
local cairo = require("lgi").cairo
local gears = require("gears")

local function awesome_icon()
    local size = theme.menu_height
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    cr:set_source(gears.color(theme.bg_focus))
    cr:paint()
    cr:set_source(gears.color(theme.fg_focus))
    cr:set_line_width(size/20)
    cr:move_to(0, size/3)
    cr:line_to(size*2/3, size/3)
    cr:move_to(size/3, size*2/3)
    cr:line_to(size*2/3, size*2/3)
    cr:line_to(size*2/3, size)
    cr:stroke()
    return img
end
theme.awesome_icon = awesome_icon()

-- Taglist squares:
local taglist_square_size = dpi(4)

local function taglist_squares_sel()
    local size = taglist_square_size
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    cr:set_source(gears.color(theme.fg_normal))
    cr:paint()
    return img
end
theme.taglist_squares_sel = taglist_squares_sel()

local function taglist_squares_unsel()
    local size = taglist_square_size
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    cr:set_source(gears.color(theme.fg_normal))
    cr:set_line_width(size/4)
    cr:rectangle(0, 0, size, size)
    cr:stroke()
    return img
end
theme.taglist_squares_unsel = taglist_squares_unsel()


local function wallpaper()
    local height = screen[1].workarea.height
    local width = screen[1].workarea.width
    local img = cairo.ImageSurface(cairo.Format.ARGB32, width, height)
    local cr = cairo.Context(img)

    local bg = xrdb.color8
    local fg = xrdb.color7
    local alt_fg = xrdb.color12
    if not is_dark_bg then
        bg, fg = fg, bg
    end

    local letter_size = height/10
    local letter_line = letter_size/18
    local letter_gap = letter_size/6
    local letter_start_x = width - width / 10
    local letter_start_y = height / 10


    local function make_letter(n, lines, color)

        local function make_line(coords)
            for i, coord in ipairs(coords) do
                if i == 1 then
                    cr:rel_move_to(coord[1], coord[2])
                else
                    cr:rel_line_to(coord[1], coord[2])
                end
            end
            cr:stroke()
        end

        lines = lines or {}
        color = color or fg
        cr:set_source(gears.color(color))
        cr:rectangle(
            letter_start_x, letter_start_y+(letter_size+letter_gap)*n,
            letter_size, letter_size
        )
        cr:fill()
        cr:set_source(gears.color(bg))
        for _, line in ipairs(lines) do
            cr:move_to(letter_start_x, letter_start_y+(letter_size+letter_gap)*n)
            make_line(line)
        end
    end

    -- bg
    cr:set_source(gears.color(bg))
    cr:paint()
    cr:set_line_width(letter_line)
    local ls = letter_size
    -- a
    make_letter(0, { {
        { 0, ls/3 },
        { ls*2/3, 0 },
    }, {
        { ls/3, ls*2/3 },
        { ls/3, 0 },
        { 0, ls/3 },
    } }, alt_fg)
    -- w
    make_letter(1, { {
        { ls/3, 0 },
        { 0,ls*2/3 },
    }, {
        { ls*2/3, 0 },
        { 0,ls*2/3 },
    } })
    -- e
    make_letter(2, { {
        { ls/3, ls/3 },
        { ls*2/3, 0 },
    }, {
        { ls/3, ls*2/3 },
        { ls*2/3, 0 },
    } })
    -- s
    make_letter(3, { {
        { ls/3, ls/3 },
        { ls*2/3, 0 },
    }, {
        { 0, ls*2/3 },
        { ls*2/3, 0 },
    } })
    -- o
    make_letter(4, { {
        { ls/2, ls/3 },
        { 0, ls/3 },
    } })
    -- m
    make_letter(5, { {
        { ls/3, ls/3 },
        { 0,ls*2/3 },
    }, {
        { ls*2/3, ls/3 },
        { 0,ls*2/3 },
    } })
    -- e
    make_letter(6, { {
        { ls/3, ls/3 },
        { ls*2/3, 0 },
    }, {
        { ls/3, ls*2/3 },
        { ls*2/3, 0 },
    } })

    return img
end
theme.wallpaper = wallpaper()

return theme
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
