local file_path, image_path = ...
require("_common_template")(...)

local cairo      = require("lgi").cairo
local pango      = require("lgi").Pango
local pangocairo = require("lgi").PangoCairo

local color     = require( "gears.color" )
local shape     = require( "gears.shape" )
local beautiful = require( "beautiful"   )

-- Run the test
local args = loadfile(file_path)() or {}

-- Draw the result
local img = cairo.SvgSurface.create(image_path..".svg", screen._get_extents() )

local cr  = cairo.Context(img)

local pango_crx = pangocairo.font_map_get_default():create_context()
local pl = pango.Layout.new(pango_crx)

-- Draw some text inside of the geometry
local function draw_label(geo, text)
    cr:save()
    cr:set_source(color(beautiful.fg_normal))
    cr:translate(geo.x, geo.y)
    pl.text = text
    cr:show_layout(pl)
    cr:restore()
end

-- Draw a mouse cursor at [x,y]
local function draw_mouse(x, y)
    cr:move_to(x, y)
    cr:rel_line_to( 0, 10)
    cr:rel_line_to( 3, -2)
    cr:rel_line_to( 3,  4)
    cr:rel_line_to( 2,  0)
    cr:rel_line_to(-3, -4)
    cr:rel_line_to( 4,  0)
    cr:close_path()
    cr:fill()
end

-- Print an outline for the screens
for _, s in ipairs(screen) do
    cr:save()
    -- Draw the screen outline
    cr:set_source(color("#00000044"))
    cr:set_line_width(1.5)
    cr:set_dash({10,4},1)
    cr:rectangle(s.geometry.x+0.75,s.geometry.y+0.75,s.geometry.width-1.5,s.geometry.height-1.5)
    cr:stroke()

    -- Draw the workarea outline
    cr:set_source(color("#00000033"))
    cr:rectangle(s.workarea.x,s.workarea.y,s.workarea.width,s.workarea.height)
    cr:stroke()

    -- Draw the padding outline
    --TODO
    cr:restore()
end

cr:set_line_width(beautiful.border_width)
cr:set_source(color(beautiful.border_color))

-- Loop each clients geometry history and paint it
for _, c in ipairs(client.get()) do
    local pgeo = nil
    for _, geo in ipairs(c._old_geo) do
        if not geo._hide then
            cr:save()
            cr:translate(geo.x, geo.y)
            shape.rounded_rect(cr, geo.width, geo.height, args.radius or 5)
            cr:stroke_preserve()
            cr:set_source(color(c.color or geo._color or beautiful.bg_normal))
            cr:fill()
            cr:restore()

            if geo._label then
                draw_label(geo, geo._label)
            end
        end

        -- Draw lines between the old and new corners
        if pgeo and not args.hide_lines then
            cr:save()
            cr:set_source_rgba(0,0,0,.1)

            -- Top left
            cr:move_to(pgeo.x, pgeo.y)
            cr:line_to(geo.x, geo.y)
            cr:stroke()

            -- Top right
            cr:move_to(pgeo.x+pgeo.width, pgeo.y)
            cr:line_to(geo.x+pgeo.width, geo.y)

            -- Bottom left
            cr:move_to(pgeo.x, pgeo.y+pgeo.height)
            cr:line_to(geo.x, geo.y+geo.height)
            cr:stroke()

            -- Bottom right
            cr:move_to(pgeo.x+pgeo.width, pgeo.y+pgeo.height)
            cr:line_to(geo.x+pgeo.width, geo.y+geo.height)
            cr:stroke()

            cr:restore()
        end

        pgeo = geo
    end
end

cr:set_source_rgba(1,0,0,1)
cr:set_dash({1,1},1)

-- Paint the mouse cursor position history
for _, h in ipairs(mouse.old_histories) do
    local pos = nil
    for _, coords in ipairs(h) do
        draw_mouse(coords.x, coords.y)
        cr:fill()

        if pos then
            cr:move_to(pos.x, pos.y)
            cr:line_to(coords.x, coords.y)
            cr:stroke()
        end

        pos = coords
    end
end

img:finish()

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
