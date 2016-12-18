--------------------------------------------------
-- Generate vector assets using current colors: --
-- (2015) Yauhen Kirylau                        --
--------------------------------------------------

local cairo = require("lgi").cairo
local gears = require("gears")
local recolor_image = gears.color.recolor_image
local screen = screen

local theme_assets = {}


function theme_assets.awesome_icon(size, bg, fg)
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    cr:set_source(gears.color(bg))
    cr:paint()
    cr:set_source(gears.color(fg))
    cr:set_line_width(size/20)
    cr:move_to(0, size/3)
    cr:line_to(size*2/3, size/3)
    cr:move_to(size/3, size*2/3)
    cr:line_to(size*2/3, size*2/3)
    cr:line_to(size*2/3, size)
    cr:stroke()
    return img
end

-- Taglist squares:
function theme_assets.taglist_squares_sel(size, fg)
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    cr:set_source(gears.color(fg))
    cr:paint()
    return img
end

function theme_assets.taglist_squares_unsel(size, fg)
    local img = cairo.ImageSurface(cairo.Format.ARGB32, size, size)
    local cr = cairo.Context(img)
    cr:set_source(gears.color(fg))
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
    cr:set_source(gears.color(color))
    cr:rectangle(
        0, (size+letter_gap)*n,
        size, size
    )
    cr:fill()

    if bg then
        cr:set_source(gears.color(bg))
    else
        cr:set_operator(cairo.Operator.CLEAR)
    end

    for _, line in ipairs(lines) do
        cr:move_to(0, (size+letter_gap)*n)
        make_line(line)
    end

    cr:set_operator(cairo.Operator.OVER)
end

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

function theme_assets.wallpaper(bg, fg, alt_fg, s)
    s = s or screen.primary
    local height = s.workarea.height
    local width = s.workarea.width
    local img = cairo.RecordingSurface(cairo.Content.COLOR,
        cairo.Rectangle { x = 0, y = 0, width = width, height = height })
    local cr = cairo.Context(img)

    local letter_start_x = width - width / 10
    local letter_start_y = height / 10
    cr:translate(letter_start_x, letter_start_y)

    -- background
    cr:set_source(gears.color(bg))
    cr:paint()

    theme_assets.gen_awesome_name(cr, height, bg, fg, alt_fg)

    return img
end

-- Recolor titlebar icons:

function theme_assets.recolor_titlebar_normal(theme, color)
    for _, titlebar_icon in ipairs({
        'titlebar_close_button_normal',
        'titlebar_minimize_button_normal',
        'titlebar_ontop_button_normal_inactive',
        'titlebar_ontop_button_normal_active',
        'titlebar_sticky_button_normal_inactive',
        'titlebar_sticky_button_normal_active',
        'titlebar_floating_button_normal_inactive',
        'titlebar_floating_button_normal_active',
        'titlebar_maximized_button_normal_inactive',
        'titlebar_maximized_button_normal_active',
    }) do
        theme[titlebar_icon] = recolor_image(theme[titlebar_icon], color)
    end
    return theme
end

function theme_assets.recolor_titlebar_focus(theme, color)
    for _, titlebar_icon in ipairs({
        'titlebar_close_button_focus',
        'titlebar_minimize_button_focus',
        'titlebar_ontop_button_focus_inactive',
        'titlebar_ontop_button_focus_active',
        'titlebar_sticky_button_focus_inactive',
        'titlebar_sticky_button_focus_active',
        'titlebar_floating_button_focus_inactive',
        'titlebar_floating_button_focus_active',
        'titlebar_maximized_button_focus_inactive',
        'titlebar_maximized_button_focus_active',
    }) do
        theme[titlebar_icon] = recolor_image(theme[titlebar_icon], color)
    end
    return theme
end

-- Recolor layout icons:
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
