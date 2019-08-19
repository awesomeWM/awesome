-- This template draw the screens with detailed information about the size
-- of various reserved areas such as struts, workareas, padding and *boxes.

local file_path, image_path = ...
require("_common_template")(...)
screen._track_workarea = true

local cairo = require("lgi").cairo
local Pango = require("lgi").Pango
local PangoCairo = require("lgi").PangoCairo
local color = require("gears.color")

-- Let the test request a size and file format
local args = loadfile(file_path)() or 10
args = args or {}
args.factor = args.factor or 10

local factor, img, cr = 1/args.factor

require("gears.timer").run_delayed_calls_now()

-- A second pass will add rulers with measurements on top.
local vrulers, hrulers = {}, {}

local function sur_to_pat(img2)
    local pat = cairo.Pattern.create_for_surface(img2)
    pat:set_extend(cairo.Extend.REPEAT)
    return pat
end

-- Imported for elv13/blind/pattern.lua
local function stripe_pat(col, angle, line_width, spacing)
    col        = color(col)
    angle      = angle or math.pi/4
    line_width = line_width or 1
    spacing    = spacing or line_width

    local hy = line_width + 2*spacing

    -- Get the necessary width and height so the line repeat itself correctly
    local a, o = math.cos(angle)*hy, math.sin(angle)*hy

    --FIXME spacing need to be in "w", not "hy"
    local w, h = math.ceil(a + (line_width - 1)), math.ceil(o + (line_width - 1))
--     ajust_size(self, w, h) --FIXME need a "force_size" method

    -- Create the pattern
    local img2 = cairo.SvgSurface.create(nil, w, h)
    local cr2  = cairo.Context(img2)

    -- Avoid artefacts caused by anti-aliasing
    local offset = line_width

    -- Setup
    cr2:set_source(color(col))
    cr2:set_line_width(line_width)

    -- The central line
    cr2:move_to(-offset, -offset)
    cr2:line_to(w+offset, h+offset)
    cr2:stroke()

    --FIXME sin/cos required for this to work with other angles than 45 degree

    -- Top right
    cr2:move_to(-offset + w - spacing/2+line_width, -offset)
    cr2:line_to(2*w+offset - spacing/2+line_width, h+offset)
    cr2:stroke()

    -- Bottom left
    cr2:move_to(-offset + spacing/2-line_width, -offset + h)
    cr2:line_to(w+offset + spacing/2-line_width, 2*h+offset)
    cr2:stroke()

    return sur_to_pat(img2)
end

local colors = {
    geometry    = "#000000",
    workarea    = "#0000ff",
    tiling_area = "#ff0000",
}

local function draw_area(_, rect, name, offset, highlight)
    local x, y = rect.x*factor+offset, rect.y*factor+offset
    cr:rectangle(x, y, rect.width*factor, rect.height*factor)

    if highlight then
        cr:save()
        cr:set_source(stripe_pat(colors[name].."22", nil, 1, 3))
        cr:fill_preserve()
        cr:restore()
    end

    cr:set_source(color(colors[name].."44"))
    cr:stroke()
end

local function compute_ruler(_, rect, name)
    table.insert(hrulers, {
        label = name, x = rect.x, width = rect.width
    })
    table.insert(vrulers, {
        label = name, y = rect.y, height = rect.height
    })
end

local function bubble_sort(t, var1, var2)
    for i=1, #t do
        if t[i+1] and t[i][var1] < t[i+1][var1] then
            t[i+1], t[i] = t[i], t[i+1]
            return false
        end

        if t[i+1] and t[i][var1] == t[i+1][var1] and t[i][var2] < t[i+1][var2] then
            t[i+1], t[i] = t[i], t[i+1]
            return false
        end
    end

    return true
end

local dx = 5

local function show_ruler_label(offset, padding, ruler, playout)
    local lbl

    if ruler.x then
        lbl = "<b>"..ruler.label..":</b>    <i>x = "..
            ruler.x.."    width = "..ruler.width.."</i>"
    else
        lbl = "<b>"..ruler.label..":</b>    <i>y = "..
            ruler.y.."    height = "..ruler.height.."</i>"
    end

    local attr, parsed = Pango.parse_markup(lbl, -1, 0)
    playout.attributes, playout.text = attr, parsed
    local _, logical = playout:get_pixel_extents()
    cr:move_to((offset.x - logical.width) /2, offset.y+padding)
    cr:show_layout(playout)
end

local function draw_rulers(s)
    -- The table has a maximum of 4 entries, the sort algorithm is irrelevant.
    while not bubble_sort(hrulers, "x", "width" ) do end
    while not bubble_sort(vrulers, "y", "height") do end

    cr:set_line_width(1)
    cr:set_dash(nil)

    local sx = (s.geometry.x+s.geometry.width )*factor
    local sy = (s.geometry.y+s.geometry.height)*factor


    local pctx    = PangoCairo.font_map_get_default():create_context()
    local playout = Pango.Layout.new(pctx)
    local pdesc   = Pango.FontDescription()
    pdesc:set_absolute_size(11 * Pango.SCALE)
    playout:set_font_description(pdesc)
    local attr, parsed = Pango.parse_markup("<b>GeometryWorkareaPaddingMargins</b>", -1, 0)
    playout.attributes, playout.text = attr, parsed
    local _, logical = playout:get_pixel_extents()
    dx = logical.height + 10

    for k, ruler in ipairs(vrulers) do
        local pad = 5+(k-1)*dx
        cr:set_source(color(colors[ruler.label].."66"))
        cr:move_to(sx+k*dx, ruler.y*factor)
        cr:line_to(sx+k*dx, ruler.y*factor+ruler.height*factor)
        cr:stroke()

        cr:move_to(sx+k*dx-2.5,ruler.y*factor)
        cr:line_to(sx+k*dx+2.5, ruler.y*factor)
        cr:stroke()

        cr:move_to(sx+k*dx-2.5,ruler.y*factor+ruler.height*factor)
        cr:line_to(sx+k*dx+2.5, ruler.y*factor+ruler.height*factor)
        cr:stroke()

        cr:save()
        cr:move_to(sx+k*dx-2.5,ruler.y*factor)
        cr:rotate(-math.pi/2)
        show_ruler_label({x=-s.geometry.height*factor, y=sx}, pad, ruler, playout)
        cr:restore()
    end


    for k, ruler in ipairs(hrulers) do
        local pad = 10+(k-1)*dx
        cr:set_source(color(colors[ruler.label].."66"))
        cr:move_to(ruler.x*factor, sy+pad)
        cr:line_to(ruler.x*factor+ruler.width*factor, sy+pad)
        cr:stroke()

        cr:move_to(ruler.x*factor, sy+pad-2.5)
        cr:line_to(ruler.x*factor, sy+pad+2.5)
        cr:stroke()

        cr:move_to(ruler.x*factor+ruler.width*factor, sy+pad-2.5)
        cr:line_to(ruler.x*factor+ruler.width*factor, sy+pad+2.5)
        cr:stroke()

        show_ruler_label({x=s.geometry.width*factor, y=sy}, pad, ruler, playout)
    end
end

local function draw_info(s)
    cr:set_source_rgb(0, 0, 0)

    local pctx    = PangoCairo.font_map_get_default():create_context()
    local playout = Pango.Layout.new(pctx)
    local pdesc   = Pango.FontDescription()
    pdesc:set_absolute_size(11 * Pango.SCALE)
    playout:set_font_description(pdesc)

    local rows = {
        "primary", "index", "geometry", "dpi", "dpi range", "outputs"
    }

    local dpi_range = s.minimum_dpi and s.preferred_dpi and s.maximum_dpi
        and (s.minimum_dpi.."-"..s.preferred_dpi.."-"..s.maximum_dpi)
        or s.dpi.."-"..s.dpi

    local geo = s.geometry

    local values = {
        screen.primary == s and "true" or "false",
        s.index,
        geo.x..":"..geo.y.." "..geo.width.."x"..geo.height,
        s.dpi,
        dpi_range,
        "",
    }

    for n, o in pairs(s.outputs) do
        table.insert(rows, "  "..n)
        table.insert(values,
            math.ceil(o.mm_width).."mm x "..math.ceil(o.mm_height).."mm"
        )
    end

    local col1_width, col2_width, height = 0, 0, 0

    -- Get the extents of the longest label.
    for k, label in ipairs(rows) do
        local attr, parsed = Pango.parse_markup(label..":", -1, 0)
        playout.attributes, playout.text = attr, parsed
        local _, logical = playout:get_pixel_extents()
        col1_width = math.max(col1_width, logical.width+10)

        attr, parsed = Pango.parse_markup(values[k], -1, 0)
        playout.attributes, playout.text = attr, parsed
        _, logical = playout:get_pixel_extents()
        col2_width = math.max(col2_width, logical.width+10)

        height = math.max(height, logical.height)
    end

    local dx2 = (s.geometry.width*factor - col1_width - col2_width - 5)/2
    local dy  = (s.geometry.height*factor - #values*height)/2 - height

    -- Draw everything.
    for k, label in ipairs(rows) do
        local attr, parsed = Pango.parse_markup(label..":", -1, 0)
        playout.attributes, playout.text = attr, parsed
        cr:move_to(dx2, dy)
        cr:show_layout(playout)

        attr, parsed = Pango.parse_markup(values[k], -1, 0)
        playout.attributes, playout.text = attr, parsed
        local _, logical = playout:get_pixel_extents()
        cr:move_to( dx2+col1_width+5, dy)
        cr:show_layout(playout)

        dy = dy + 5 + logical.height
    end
end

-- Compute the rulers size.
for _=1, screen.count() do
    local s = screen[1]

    -- The outer geometry.
    compute_ruler(s, s.geometry, "geometry")

    -- The workarea.
    compute_ruler(s, s.workarea, "workarea")

    -- The padding.
    compute_ruler(s, s.tiling_area, "tiling_area")
end

-- Get the final size of the image.
local sew, seh = screen._get_extents()
sew, seh = sew/args.factor + (screen.count()-1)*10+2, seh/args.factor+2

sew,seh=sew+100,seh+100

img = cairo.SvgSurface.create(image_path..".svg", sew, seh)
cr  = cairo.Context(img)

cr:set_line_width(1.5)
cr:set_dash({10,4},1)

-- Draw the various areas.
for k=1, screen.count() do
    local s = screen[1]

    -- The outer geometry.
    draw_area(s, s.geometry, "geometry", (k-1)*10, args.highlight_geometry)

    -- The workarea.
    draw_area(s, s.workarea, "workarea", (k-1)*10, args.highlight_workarea)

    -- The padding.
    draw_area(s, s.tiling_area, "tiling_area", (k-1)*10, args.highlight_tiling_area)

    -- Draw the ruler.
    draw_rulers(s)

    -- Draw the informations.
    if args.display_screen_info ~= false then
        draw_info(s)
    end
end

img:finish()

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
