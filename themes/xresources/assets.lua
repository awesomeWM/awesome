--------------------------------------------------
-- Generate vector assets using current colors: --
-- (2015) Yauhen Kirylau                        --
--------------------------------------------------

local cairo = require("lgi").cairo
local gears = require("gears")
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


function theme_assets.wallpaper(bg, fg, alt_fg)
    local height = screen[1].workarea.height
    local width = screen[1].workarea.width
    local img = cairo.ImageSurface(cairo.Format.ARGB32, width, height)
    local cr = cairo.Context(img)

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

return theme_assets
-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
