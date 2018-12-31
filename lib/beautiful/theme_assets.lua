----------------------------------------------------------------------------
--- Generate vector assets using current colors.
--
-- @author Yauhen Kirylau &lt;yawghen@gmail.com&gt;
-- @copyright 2015 Yauhen Kirylau
-- @submodule beautiful
----------------------------------------------------------------------------

local cairo = require("lgi").cairo
local surface = require("gears.surface")
local gears_color = require("gears.color")
local recolor_image = gears_color.recolor_image
local screen = screen

local theme_assets = {}


--- Generate selected taglist square.
-- @tparam number size Size.
-- @tparam color fg Background color.
-- @return Image with the square.
function theme_assets.taglist_squares_sel(size, fg)
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    cr:set_source(gears_color(fg))
    cr:paint()
    return img
end

--- Generate unselected taglist square.
-- @tparam number size Size.
-- @tparam color fg Background color.
-- @return Image with the square.
function theme_assets.taglist_squares_unsel(size, fg)
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    cr:set_source(gears_color(fg))
    cr:set_line_width(size/4)
    cr:rectangle(0, 0, size, size)
    cr:stroke()
    return img
end

local function make_letter(cr, n, lines, size, bg, fg, alt_fg)
    local letter_gap  = size/6

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
    local color = alt_fg or fg
    cr:set_source(gears_color(color))
    cr:rectangle(
        0, (size+letter_gap)*n,
        size, size
    )
    cr:fill()

    if bg then
        cr:set_source(gears_color(bg))
    else
        cr:set_operator(cairo.Operator.CLEAR)
    end

    for _, line in ipairs(lines) do
        cr:move_to(0, (size+letter_gap)*n)
        make_line(line)
    end

    cr:set_operator(cairo.Operator.OVER)
end

--- Put Awesome WM name onto cairo surface.
-- @param cr Cairo surface.
-- @tparam number height Height.
-- @tparam color bg Background color.
-- @tparam color fg Main foreground color.
-- @tparam color alt_fg Accent foreground color.
function theme_assets.gen_awesome_name(cr, height, bg, fg, alt_fg)
    local ls = height/10 -- letter_size
    local letter_line = ls/18

    cr:set_line_width(letter_line)

    -- a
    make_letter(cr, 0, { {
        { 0, ls/3 },
        { ls*2/3, 0 },
    }, {
        { ls/3, ls*2/3 },
        { ls/3, 0 },
        { 0, ls/3 },
    } }, ls, bg, fg, alt_fg)
    -- w
    make_letter(cr, 1, { {
        { ls/3, 0 },
        { 0,ls*2/3 },
    }, {
        { ls*2/3, 0 },
        { 0,ls*2/3 },
    } }, ls, bg, fg)
    -- e
    make_letter(cr, 2, { {
        { ls/3, ls/3 },
        { ls*2/3, 0 },
    }, {
        { ls/3, ls*2/3 },
        { ls*2/3, 0 },
    } }, ls, bg, fg)
    -- s
    make_letter(cr, 3, { {
        { ls/3, ls/3 },
        { ls*2/3, 0 },
    }, {
        { 0, ls*2/3 },
        { ls*2/3, 0 },
    } }, ls, bg, fg)
    -- o
    make_letter(cr, 4, { {
        { ls/2, ls/3 },
        { 0, ls/3 },
    } }, ls, bg, fg)
    -- m
    make_letter(cr, 5, { {
        { ls/3, ls/3 },
        { 0,ls*2/3 },
    }, {
        { ls*2/3, ls/3 },
        { 0,ls*2/3 },
    } }, ls, bg, fg)
    -- e
    make_letter(cr, 6, { {
        { ls/3, ls/3 },
        { ls*2/3, 0 },
    }, {
        { ls/3, ls*2/3 },
        { ls*2/3, 0 },
    } }, ls, bg, fg)
end

--- Put Awesome WM logo onto cairo surface.
-- @param cr Cairo surface.
-- @tparam number width Width.
-- @tparam number height Height.
-- @tparam color bg Background color.
-- @tparam color fg Foreground color.
function theme_assets.gen_logo(cr, width, height, bg, fg)
    local ls = math.min(width, height)

    local letter_line = ls/18

    cr:set_line_width(letter_line)

    make_letter(cr, 0, { {
        { 0, ls/3 },
        { ls*2/3, 0 },
    }, {
        { ls/3, ls*2/3 },
        { ls/3, 0 },
        { 0, ls/3 },
    } }, ls, bg, fg)
end

--- Generate Awesome WM logo.
-- @tparam number size Size.
-- @tparam color bg Background color.
-- @tparam color fg Background color.
-- @return Image with the logo.
function theme_assets.awesome_icon(size, bg, fg)
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    theme_assets.gen_logo(cr, size, size, fg, bg)
    return img
end

--- Generate Awesome WM wallpaper.
-- @tparam color bg Background color.
-- @tparam color fg Main foreground color.
-- @tparam color alt_fg Accent foreground color.
-- @tparam screen s Screen (to get wallpaper size).
-- @return Wallpaper image.
function theme_assets.wallpaper(bg, fg, alt_fg, s)
    s = s or screen.primary
    local height = s.geometry.height
    local width = s.geometry.width
    local img = cairo.RecordingSurface(cairo.Content.COLOR,
        cairo.Rectangle { x = 0, y = 0, width = width, height = height })
    local cr = cairo.Context(img)

    local letter_start_x = width - width / 10
    local letter_start_y = height / 10
    cr:translate(letter_start_x, letter_start_y)

    -- background
    cr:set_source(gears_color(bg))
    cr:paint()

    theme_assets.gen_awesome_name(cr, height, bg, fg, alt_fg)

    return img
end

--- Recolor titlebar icons.
-- @tparam table theme Beautiful theme table.
-- @tparam color color Icons' color.
-- @tparam string state `"normal"` or `"focus"`.
-- @tparam string postfix `nil`, `"hover"` or `"press"`.
-- @treturn table Beautiful theme table with the images recolored.
function theme_assets.recolor_titlebar(theme, color, state, postfix)
    if postfix then postfix='_'..postfix end
    for _, titlebar_icon_name in ipairs({
        'titlebar_close_button_'..state..'',
        'titlebar_minimize_button_'..state..'',
        'titlebar_ontop_button_'..state..'_inactive',
        'titlebar_ontop_button_'..state..'_active',
        'titlebar_sticky_button_'..state..'_inactive',
        'titlebar_sticky_button_'..state..'_active',
        'titlebar_floating_button_'..state..'_inactive',
        'titlebar_floating_button_'..state..'_active',
        'titlebar_maximized_button_'..state..'_inactive',
        'titlebar_maximized_button_'..state..'_active',
    }) do
        local full_name = postfix and (
            titlebar_icon_name .. postfix
        ) or titlebar_icon_name
        local image = theme[full_name] or theme[titlebar_icon_name]
        if image then
            image = surface.duplicate_surface(image)
            theme[full_name] = recolor_image(image, color)
        end
    end
    return theme
end


--- Recolor unfocused titlebar icons.
-- This method is deprecated.  Use a `beautiful.theme_assets.recolor_titlebar`.
-- @tparam table theme Beautiful theme table
-- @tparam color color Icons' color.
-- @treturn table Beautiful theme table with the images recolored.
-- @deprecated recolor_titlebar_normal
function theme_assets.recolor_titlebar_normal(theme, color)
    return theme_assets.recolor_titlebar(theme, color, "normal")
end

--- Recolor focused titlebar icons.
-- This method is deprecated.  Use a `beautiful.theme_assets.recolor_titlebar`.
-- @tparam table theme Beautiful theme table
-- @tparam color color Icons' color.
-- @treturn table Beautiful theme table with the images recolored.
-- @deprecated recolor_titlebar_focus
function theme_assets.recolor_titlebar_focus(theme, color)
    return theme_assets.recolor_titlebar(theme, color, "focus")
end

--- Recolor layout icons.
-- @tparam table theme Beautiful theme table
-- @tparam color color Icons' color.
-- @treturn table Beautiful theme table with the images recolored.
function theme_assets.recolor_layout(theme, color)
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
        theme[layout_name] = recolor_image(theme[layout_name], color)
    end
    return theme
end

return theme_assets
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
