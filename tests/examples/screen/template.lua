-- This template draw the screens with detailed information about the size
-- of various reserved areas such as struts, workareas, padding and *boxes.

local file_path, image_path = ...
require("_common_template")(...)
screen._track_workarea = true

local cairo = require("lgi").cairo
local Pango = require("lgi").Pango
local PangoCairo = require("lgi").PangoCairo
local color = require("gears.color")
local aclient = require("awful.client")
local beautiful = require("beautiful")

-- Let the test request a size and file format
local args = loadfile(file_path)() or 10
args = args or {}
args.factor = args.factor or 10

local SCALE_FACTOR = 0.66

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
--     adjust_size(self, w, h) --FIXME need a "force_size" method

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
    geometry      = beautiful.fg_normal,
    workarea      = "#0000ff",
    tiling_area   = "#ff0000",
    padding_area  = "#ff0000",
    wibar         = beautiful.fg_normal,
    tiling_client = "#ff0000",
    gaps          = "#9900ff",
}

local function draw_area(_, rect, name, offset, highlight)
    local x, y = rect.x*factor+offset, rect.y*factor+offset
    cr:rectangle(x, y, rect.width*factor, rect.height*factor)

    if highlight then
        cr:save()
        cr:set_source(stripe_pat(color.change_opacity(colors[name], 0.0863), nil, 1, 3))
        cr:fill_preserve()
        cr:restore()
    end

    cr:set_source(color.change_opacity(colors[name], 0.1725))
    cr:stroke()
end

local function draw_bounding_area(_, rect, hole, name, offset)
    draw_area(_, rect, name, offset, true)

    local x, y = hole.x*factor+offset, hole.y*factor+offset
    cr:set_operator(cairo.Operator.CLEAR)
    cr:rectangle(x, y, hole.width*factor, hole.height*factor)
    cr:set_source_rgb(1, 1, 1)
    cr:fill()
    cr:set_operator(cairo.Operator.OVER)
end

local function draw_solid_area(_, rect, name, offset, alpha)
    alpha = alpha or 0.2314 -- Defaults to 35%
    local x, y = rect.x*factor+offset, rect.y*factor+offset
    cr:rectangle(x, y, rect.width*factor, rect.height*factor)

    cr:save()
    cr:set_source(color.change_opacity(colors[name], alpha))
    cr:fill_preserve()
    cr:restore()

    cr:set_source(color.change_opacity(colors[name], 0.0863))
    cr:stroke()
end

local function write_on_area_middle(rect, text, offset)
    -- Prepare to write on the rect area
    local pctx    = PangoCairo.font_map_get_default():create_context()
    local playout = Pango.Layout.new(pctx)
    local pdesc   = Pango.FontDescription()
    pdesc:set_absolute_size(11 * Pango.SCALE)
    playout:set_font_description(pdesc)

    -- Write 'text' on the rect area
    playout.attributes, playout.text = Pango.parse_markup(text, -1, 0)
    local _, logical = playout:get_pixel_extents()
    local dx = (rect.x*factor+offset) + (rect.width*factor - logical.width) / 2
    local dy  = (rect.y*factor+offset) + (rect.height*factor - logical.height) / 2
    cr:set_source(beautiful.fg_normal)
    cr:move_to(dx, dy)
    cr:show_layout(playout)
end

-- For clients/wibars with struts only.
local function draw_struct(_, struct, name, offset, label)
    draw_solid_area(_, struct, name, offset)
    if type(label) == 'string' then
        write_on_area_middle(struct, label, offset)
    end
end

-- For floating or tiled clients.
local function draw_client(_, c, name, offset, label, alpha)
    draw_solid_area(_, c, name, offset, alpha)
    if type(label) == 'string' then
        write_on_area_middle(c, label, offset)
    end
end


local function compute_ruler(s, rect, name)
    hrulers[s], vrulers[s] = hrulers[s] or {}, vrulers[s] or {}
    table.insert(hrulers[s], {
        label = name, x = rect.x, width = rect.width
    })
    table.insert(vrulers[s], {
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

local playout, playout_height = nil, nil

local function get_playout()
    if playout then return playout end

    local pctx    = PangoCairo.font_map_get_default():create_context()
    playout       = Pango.Layout.new(pctx)
    local pdesc   = Pango.FontDescription()
    pdesc:set_absolute_size(11 * Pango.SCALE)
    playout:set_font_description(pdesc)

    return playout
end

local function get_text_height()
    if playout_height then return playout_height end

    local l = get_playout()
    local attr, parsed = Pango.parse_markup("<b>GeometryWorkareaPaddingMargins</b>", -1, 0)
    l.attributes, l.text = attr, parsed

    local _, logical = playout:get_pixel_extents()

    playout_height = logical.height

    return playout_height
end

local function show_ruler_label(offset, padding, ruler, playout2)
    if not ruler.label then return end

    local lbl

    if ruler.x then
        lbl = "<b>"..ruler.label..":</b>    <i>x = "..
            ruler.x.."    width = "..ruler.width.."</i>"
    else
        lbl = "<b>"..ruler.label..":</b>    <i>y = "..
            ruler.y.."    height = "..ruler.height.."</i>"
    end

    local attr, parsed = Pango.parse_markup(lbl, -1, 0)
    playout2.attributes, playout2.text = attr, parsed
    local _, logical = playout2:get_pixel_extents()
    cr:move_to((offset.x - logical.width) /2, offset.y+padding)
    cr:show_layout(playout2)
end

local function show_aligned_label(dx2, offset, padding, ruler)
    local lbl

    if ruler.x then
        lbl = ruler.width
    else
        lbl = ruler.height
    end

    local attr, parsed = Pango.parse_markup(lbl, -1, 0)
    playout.attributes, playout.text = attr, parsed
    local _, logical = playout:get_pixel_extents()

    if ruler.x then
        local off = (ruler.width*factor - logical.width)/2
        cr:move_to(ruler.x*factor+off, offset.y+padding)
    else
        local off = (ruler.height*factor - logical.width)/2
        cr:move_to(-ruler.y*factor-dx2*factor-off, padding)
    end

    cr:show_layout(playout)
end

local function draw_vruler(s, dx2, sx, ruler, layer)
    local pad = 5+(layer-1)*dx2
    cr:set_source(color(ruler.color or (color.change_opacity(colors[ruler.label], 0.2588))))
    cr:move_to(sx+layer*dx2, ruler.y*factor)
    cr:line_to(sx+layer*dx2, ruler.y*factor+ruler.height*factor)
    cr:stroke()

    cr:move_to(sx+layer*dx2-2.5,ruler.y*factor)
    cr:line_to(sx+layer*dx2+2.5, ruler.y*factor)
    cr:stroke()

    cr:move_to(sx+layer*dx2-2.5,ruler.y*factor+ruler.height*factor)
    cr:line_to(sx+layer*dx2+2.5, ruler.y*factor+ruler.height*factor)
    cr:stroke()

    cr:save()
    cr:move_to(sx+layer*dx2-2.5,ruler.y*factor)
    cr:rotate(-math.pi/2)
    if ruler and ruler.label then
        show_ruler_label({x=-s.geometry.height*factor, y=sx}, pad, ruler, get_playout())
    elseif ruler and ruler.align then
        show_aligned_label(dx2, {x=s.geometry.width*factor, y=0}, pad, ruler)
    end
    cr:restore()
end

local function draw_hruler(s, dx2, sy, ruler, layer)
    local pad = 10+(layer-1)*(dx2 or 0)
    cr:set_source(color(ruler.color or (color.change_opacity(colors[ruler.label], 0.2588))))
    cr:move_to(ruler.x*factor, sy+pad)
    cr:line_to(ruler.x*factor+ruler.width*factor, sy+pad)
    cr:stroke()

    cr:move_to(ruler.x*factor, sy+pad-2.5)
    cr:line_to(ruler.x*factor, sy+pad+2.5)
    cr:stroke()

    cr:move_to(ruler.x*factor+ruler.width*factor, sy+pad-2.5)
    cr:line_to(ruler.x*factor+ruler.width*factor, sy+pad+2.5)
    cr:stroke()

    if ruler and ruler.label then
        show_ruler_label({x=s.geometry.width*factor, y=sy}, pad, ruler, get_playout())
    elseif ruler and ruler.align then
        show_aligned_label(dx2, {x=s.geometry.width*factor, y=sy}, pad, ruler)
    end
end

local function draw_rulers(s)
    -- The table has a maximum of 4 entries, the sort algorithm is irrelevant.
    while not bubble_sort(hrulers[s], "x", "width" ) do end
    while not bubble_sort(vrulers[s], "y", "height") do end

    cr:set_line_width(1)
    cr:set_dash(nil)

    local sx = (s.geometry.x+s.geometry.width )*factor
    local sy = (s.geometry.y+s.geometry.height)*factor

    dx = get_text_height() + 10

    for k, ruler in ipairs(vrulers[s]) do
        draw_vruler(s, dx, sx, ruler, k)
    end

    for k, ruler in ipairs(hrulers) do
        draw_hruler(s, dx, sy, ruler, k)
    end
end

local tr_x, tr_y = 0, 0

-- Not a very efficient way to do this, but at least it is simple.
local function deduplicate_gaps(gaps)
    for _, gap1 in ipairs(gaps) do
        for k, gap2 in ipairs(gaps) do
            if gap1[2] == gap2[1] then
                gap1[2] = gap2[2]
                table.remove(gaps, k)
                return true
            elseif gap2[1] >= gap1[1] and gap2[2] <= gap1[2] and gap1 ~= gap2 then
                table.remove(gaps, k)
                return true
            elseif gap1[1] == gap2[1] and  gap2[2] == gap2[2] and gap1 ~= gap2 then
                table.remove(gaps, k)
                return true
            end
        end
    end


    return false
end

local function get_gaps(s)
    local ret = {vertical={gaps={}, content={}}, horizontal={gaps={}, content={}}}

    local gap = s.selected_tag.gap

    if gap == 0 then return ret end

    if s.selected_tag.gap_single_client == false and #s.tiled_clients == 1 then
        return ret
    end

    -- First, get all gaps.
    for _, c in ipairs(s.tiled_clients) do
        local bw = c.border_width
        table.insert(ret.horizontal.gaps, {c.x-gap     , c.x             })
        table.insert(ret.vertical.gaps  , {c.y-gap     , c.y             })
        table.insert(ret.horizontal.gaps, {c.x+c.width +2*bw, c.x+c.width+gap +2*bw})
        table.insert(ret.vertical.gaps  , {c.y+c.height+2*bw, c.y+c.height+gap+2*bw})
    end

    -- Merge continuous gaps.
    while deduplicate_gaps(ret.vertical.gaps  ) do end
    while deduplicate_gaps(ret.horizontal.gaps) do end

    return ret
end

local function evaluate_translation(draw_gaps, draw_struts, draw_mwfact, draw_client_snap)
    for s in screen do
        if (draw_gaps and s.selected_tag and s.selected_tag.gap > 0) then
            local gaps = get_gaps(s)

            -- Only add the space if there is something to display.
            if #gaps.horizontal.gaps > 0 then
                tr_y = math.max(tr_y, 3 * get_text_height())
            end

            if #gaps.vertical.gaps > 0 then
                tr_x = math.max(tr_x, 2 * get_text_height())
            end
        end

        if draw_client_snap or draw_struts then
            tr_y = math.max(tr_y, 3 * get_text_height())
            tr_x = math.max(tr_x, 2 * get_text_height())
        end

        if draw_mwfact then
            tr_y = math.max(tr_y, 3 * get_text_height())
        end
    end
end

local function draw_gaps(s)
    cr:translate(-tr_x, -tr_y)

    local gaps = get_gaps(s)

    local offset = s.tiling_area

    for _, hgap in ipairs(gaps.horizontal.gaps) do
        draw_hruler(
            s,
            offset.x,
            get_text_height(),
            {
                x     = offset.x+hgap[1]+tr_x,
                width = math.ceil(hgap[2]-hgap[1]),
                label = nil,
                color = color.change_opacity(colors.gaps, 0.2588),
                align = true
            },
            1
         )
    end

    for _, vgap in ipairs(gaps.vertical.gaps) do
        draw_vruler(
            s,
            get_text_height()*1.5,
            0,
            {
                y      = offset.y+vgap[1]+tr_y,
                height = math.ceil(vgap[2]-vgap[1]),
                label  = nil,
                color  = color.change_opacity(colors.gaps, 0.2588),
                align  = true
            },
            1
        )
    end

    cr:translate(tr_x, tr_y)
end

local function has_struts(s)
    for k, v in pairs(s.workarea) do
        if s.geometry[k] ~= v then
            return true
        end
    end

    return false
end

local function draw_struts(s)
    local left   = s.workarea.x - s.geometry.x
    local right  = (s.geometry.x + s.geometry.width) - (s.workarea.x + s.workarea.width)
    local top    = s.workarea.y - s.geometry.y
    local bottom = (s.geometry.y + s.geometry.height) - (s.workarea.y + s.workarea.height)

    cr:translate(-tr_x, -tr_y)

    if left > 0 then
        draw_hruler(
            s,
            s.geometry.y*SCALE_FACTOR,
            get_text_height(),
            {
                x     = s.geometry.x+tr_x*2,
                width = left,
                color = color.change_opacity(colors.gaps, 0.2588),
                align = true
            },
            1
        )
    end

    if top > 0 then
        draw_vruler(
            s,
            get_text_height()*1.5,
            s.geometry.x*SCALE_FACTOR,
            {
                y      = s.geometry.y+tr_y*(1/factor),
                height = top,
                color  = color.change_opacity(colors.gaps, 0.2588),
                align  = true
            },
            1
        )
    end

    if right > 0 then
        draw_hruler(
            s,
            s.geometry.y*SCALE_FACTOR,
            get_text_height(),
            {
                x     = s.geometry.x,
                width = left,
                color = color.change_opacity(colors.gaps, 0.2588),
                align = true
            },
            1
        )
    end

    if bottom > 0 then
        draw_vruler(
            s,
            get_text_height()*1.5,
            s.geometry.x*SCALE_FACTOR,
            {
                y      = s.geometry.y+tr_y*(1/factor)+s.geometry.height - bottom,
                height = bottom,
                color  = color.change_opacity(colors.gaps, 0.2588),
                align  = true
            },
            1
        )
    end

    cr:translate(tr_x, tr_y)
end

local function draw_mwfact(s)
    cr:translate(-tr_x, -tr_y)

    local mwfact = s.selected_tag.master_width_factor

    local offset = s.tiling_area.x
    local width  = s.tiling_area.width

    local w1, w2 = math.ceil(width*mwfact), math.ceil(width*(1-mwfact))

    draw_hruler(s, offset, get_text_height(), {
        x     = offset,
        width = w1,
        color = color.change_opacity(colors.gaps, 0.2588),
        align = true
    }, 1)

    draw_hruler(s, offset, get_text_height(), {
        x     = offset+w1,
        width = w2,
        color = color.change_opacity(colors.gaps, 0.2588),
        align = true
    }, 1)

    cr:translate(tr_x, tr_y)
end

local function draw_wfact(s)
    cr:translate(-tr_x, -tr_y)

    local tags = s.selected_tags
    local windowfacts = s.selected_tag.windowfact
    local height  = s.tiling_area.height / SCALE_FACTOR

    local sum, gap = 0, s.selected_tag.gap or 0

    for _, t in ipairs(tags) do
        for _, c in ipairs(t:clients()) do
            local info = aclient.idx(c)
            sum = sum + windowfacts[info.col][info.idx]
        end
    end

    local offset = s.tiling_area.y * args.factor + tr_y + (2*gap)

    for i = 1, #windowfacts[1] do
        draw_vruler(
            s,
            0, --s.geometry.x + s.geometry.width,
            s.geometry.width * factor + 5,
            {
                y      = math.floor(offset),
                height =math.ceil( (height/sum) * windowfacts[1][i]),
                color  = color.change_opacity(colors.gaps, 0.2588),
                align  = true,
            },
            1
        )

        offset = offset + (height/sum * windowfacts[1][i]) + (2*gap)
    end

    cr:translate(tr_x, tr_y)
end


local function draw_client_snap(s)
    cr:translate(-tr_x, -tr_y)

    local snap_areas = {
        vertical  ={},
        horizontal={}
    }

    local d = require("awful.mouse.snap").default_distance

    for _, c in ipairs(s.clients) do
        if c.floating then
            table.insert(snap_areas.horizontal, {c.x-d, c.x})
            table.insert(snap_areas.horizontal, {c.x+c.width, c.x+c.width+d})
            table.insert(snap_areas.vertical  , {c.y-d, c.y})
            table.insert(snap_areas.vertical  , {c.y+c.height, c.y+c.height+d})
        end
    end

    while deduplicate_gaps(snap_areas.horizontal) do end
    while deduplicate_gaps(snap_areas.vertical  ) do end

    --FIXME
    --[[for _, hgap in ipairs(snap_areas.horizontal) do
        draw_hruler(
            s,
            0,
            get_text_height(),
            {x=tr_x+s.workarea.x+hgap[1],width=hgap[2]-hgap[1],label="gaps"},
            1
        )
    end

    for _, vgap in ipairs(snap_areas.vertical) do
        draw_vruler(
            s,
            get_text_height()*1.5,
            0,
            {y=tr_y+vgap[1],height=vgap[2]-vgap[1],label="gaps"},
            1
        )
    end]]--

    cr:translate(tr_x, tr_y)
end

-- local function draw_screen_snap(s)
--     local d = require("awful.mouse.snap").aerosnap_distance
--
--
--     local proxy = {
--         x      = c.x - sd,
--         y      = c.y - sd,
--         width  = c.width + 2*sd,
--         height = c.height + 2*sd,
--     }
--
--     draw_client(s, proxy, 'gaps', (k-1)*10, nil, "11")
-- end

local function draw_info(s)
    cr:set_source(beautiful.fg_normal)

    local pctx     = PangoCairo.font_map_get_default():create_context()
    local playout2 = Pango.Layout.new(pctx)
    local pdesc    = Pango.FontDescription()
    pdesc:set_absolute_size(11 * Pango.SCALE)
    playout2:set_font_description(pdesc)

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
        playout2.attributes, playout2.text = attr, parsed
        local _, logical = playout2:get_pixel_extents()
        col1_width = math.max(col1_width, logical.width+10)

        attr, parsed = Pango.parse_markup(values[k], -1, 0)
        playout2.attributes, playout2.text = attr, parsed
        _, logical = playout2:get_pixel_extents()
        col2_width = math.max(col2_width, logical.width+10)

        height = math.max(height, logical.height)
    end

    local dx2 = (s.geometry.width*factor - col1_width - col2_width - 5)/2
    local dy  = (s.geometry.height*factor - #values*height)/2 - height

    -- Draw everything.
    for k, label in ipairs(rows) do
        local attr, parsed = Pango.parse_markup(label..":", -1, 0)
        playout2.attributes, playout2.text = attr, parsed
        cr:move_to(dx2, dy)
        cr:show_layout(playout2)

        attr, parsed = Pango.parse_markup(values[k], -1, 0)
        playout2.attributes, playout2.text = attr, parsed
        local _, logical = playout2:get_pixel_extents()
        cr:move_to( dx2+col1_width+5, dy)
        cr:show_layout(playout2)

        dy = dy + 5 + logical.height
    end
end

-- Compute the rulers size.
for k=1, screen.count() do
    local s = screen[k]

    -- The padding.
    compute_ruler(s, s.tiling_area, "tiling_area")

    -- The workarea.
    compute_ruler(s, s.workarea, "workarea")

    -- The outer geometry.
    compute_ruler(s, s.geometry, "geometry")
end

-- If there is some rulers on the left/top, add a global translation.
evaluate_translation(
    args.draw_gaps,
    args.draw_struts,
    args.draw_mwfact,
    args.draw_client_snap
)

-- Get the final size of the image.
local sew, seh = screen._get_extents()
sew, seh = sew/args.factor + (screen.count()-1)*10+2, seh/args.factor+2

sew, seh = sew + tr_x, seh + SCALE_FACTOR*tr_y

sew, seh = sew + 5*get_text_height(), seh + 5*get_text_height()

img = cairo.SvgSurface.create(image_path..".svg", sew, seh)
cr  = cairo.Context(img)

-- Instead of adding origin offset everywhere, translate the viewport.
cr:translate(tr_x, tr_y * SCALE_FACTOR)

-- Draw the various areas.
for k=1, screen.count() do
    local s = screen[k]

    cr:set_line_width(1.5)
    cr:set_dash({10,4},1)

    -- The outer geometry.
    draw_area(s, s.geometry, "geometry", (k-1)*10, args.highlight_geometry)

    -- The workarea.
    draw_area(s, s.workarea, "workarea", (k-1)*10, args.highlight_workarea)

    -- The padding.
    if args.highlight_padding_area then
        draw_bounding_area(s, s.workarea, s.tiling_area, "padding_area", (k-1)*10)
    end

    draw_area(s, s.tiling_area, "tiling_area", (k-1)*10, args.highlight_tiling_area)

    -- Draw the ruler.
    if args.draw_areas ~= false then
        draw_rulers(s)
    end

    -- Draw the wibar.
    for _, wibar in ipairs(args.draw_wibars or {}) do
        if wibar.screen == s then
            draw_struct(s, wibar, 'wibar', (k-1)*10, 'Wibar')
        end
    end

    local skip_gaps = s.selected_tag
        and s.selected_tag.gap_single_client == false
        and #s.tiled_clients == 1

    local sd = require("awful.mouse.snap").default_distance

    -- Draw clients.
    if args.draw_clients then
        for label,c in pairs(args.draw_clients) do
            if c.screen == s then
                local gap = c:tags()[1].gap
                if args.draw_gaps and gap > 0 and (not c.floating) and not skip_gaps then
                    local proxy = {
                        x      = c.x - gap,
                        y      = c.y - gap,
                        width  = c.width + 2*gap,
                        height = c.height + 2*gap,
                    }

                    draw_client(s, proxy, 'gaps', (k-1)*10, nil, 0.0431)
                elseif args.draw_client_snap and c.floating then
                    local proxy = {
                        x      = c.x - sd,
                        y      = c.y - sd,
                        width  = c.width + 2*sd,
                        height = c.height + 2*sd,
                    }

                    draw_client(s, proxy, 'gaps', (k-1)*10, nil, 0.0431)
                end

                draw_client(s, c, 'tiling_client', (k-1)*10, label)
            end
        end
    end

    if args.draw_struts and has_struts(s) then
        draw_struts(s)
    end

    -- Draw the informations.
    if args.display_screen_info ~= false then
        draw_info(s)
    end

    -- Draw the useless gaps.
    if args.draw_gaps and not skip_gaps then
        draw_gaps(s)
    end

    -- Draw the master width factor gaps.
    if args.draw_mwfact then
        draw_mwfact(s)
    end

    -- Draw the (rows) width factor.
    if args.draw_wfact then
        draw_wfact(s)
    end

    -- Draw the snapping areas of floating clients.
    if args.draw_client_snap then
        draw_client_snap(s)
    end

    -- Draw the screen edge areas.
    --if args.draw_screen_snap then
    --    draw_screen_snap(s)
    --end
end

img:finish()

-- vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
