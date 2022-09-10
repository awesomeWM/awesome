local file_path, image_path = ...
require("_common_template")(...)

local cairo      = require("lgi").cairo

local color     = require( "gears.color"    )
local shape     = require( "gears.shape"    )
local beautiful = require( "beautiful"      )
local wibox     = require( "wibox"          )

-- Run the test
local args = loadfile(file_path)() or {}

-- Emulate the event loop for 5 iterations
for _ = 1, 5 do
    require("gears.timer").run_delayed_calls_now()
end

-- Draw the result
local img = cairo.SvgSurface.create(image_path..".svg", screen._get_extents() )

local cr  = cairo.Context(img)

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
if not rawget(screen, "no_outline") then
    for s in screen do
        cr:save()
        -- Draw the screen outline
        if s._no_outline ~= true then
            cr:set_source(color("#00000044"))
            cr:set_line_width(1.5)
            cr:set_dash({10,4},1)
            cr:rectangle(s.geometry.x+0.75,s.geometry.y+0.75,s.geometry.width-1.5,s.geometry.height-1.5)
            cr:stroke()

            -- Draw the workarea outline
            cr:set_source(color("#00000033"))
            cr:rectangle(s.workarea.x,s.workarea.y,s.workarea.width,s.workarea.height)
            cr:stroke()
        end

        -- Draw the padding outline
        --TODO
        cr:restore()
    end
end

-- Draw the wallpaper (if any).
if root._wallpaper_pattern then
    cr:set_source_rgb(1,0,0)
    cr:set_line_width(2)
    cr:rectangle(0, 0, root.size())
    cr:stroke()
    cr:set_source(root._wallpaper_pattern)
    cr:rectangle(0, 0, root.size())
    cr:fill()
    cr:paint()
end


cr:set_line_width(beautiful.border_width/2)
cr:set_source(color(beautiful.border_color))


local rect = {x1 = 0 ,y1 = 0 , x2 = 0 , y2 = 0}

-- Get the region with wiboxes
for _, obj in ipairs {drawin, client} do
    for _, d in ipairs(obj.get()) do
        local w = d.get_wibox and d:get_wibox() or d
        if w and w.geometry and w.visible then
            local geo = w:geometry()
            rect.x1 = math.min(rect.x1, geo.x                                       )
            rect.y1 = math.min(rect.y1, geo.y                                       )
            rect.x2 = math.max(rect.x2, geo.x + geo.width  + 2*(w.border_width or 0))
            rect.y2 = math.max(rect.y2, geo.y + geo.height + 2*(w.border_width or 0))
        end
    end
end

local total_area = wibox.layout {
    forced_width  = rect.x2,
    forced_height = rect.y2,
    layout        = wibox.layout.manual,
}

local function wrap_titlebar(tb, width, height)

    local bg, fg

    if args.honor_titlebar_colors then
        bg = tb.drawable.background_color or tb.args.bg_normal
        fg = tb.drawable.foreground_color or tb.args.fg_normal
    else
        bg, fg = tb.args.bg_normal, tb.args.fg_normal
    end

    return wibox.widget {
        tb.drawable.widget,
        bg            = bg,
        fg            = fg,
        forced_width  = width,
        forced_height = height,
        widget        = wibox.container.background
    }
end

local function client_widget(c, col, label)
    local geo = c:geometry()
    local bw = c.border_width or beautiful.border_width or 0
    local bc = c.border_color or beautiful.border_color

    local l = wibox.layout.align.vertical()
    l.fill_space = true

    local tbs = c._private and c._private.titlebars or {}

    local map = {
        top    = "set_first",
        left   = "set_first",
        bottom = "set_third",
        right  = "set_third",
    }

    for _, position in ipairs{"top", "bottom"} do
        local tb = tbs[position]
        if tb then
            l[map[position]](l, wrap_titlebar(tb, c:geometry().width, tb.args.height or 16))
        end
    end

    for _, position in ipairs{"left", "right"} do
        local tb = tbs[position]
        if tb then
            l[map[position]](l, wrap_titlebar(tb, tb.args.width or 16, c:geometry().height))
        end
    end

    local l2 = wibox.layout.align.horizontal()
    l2.fill_space = true
    l:set_second(l2)
    l.forced_width  = c.width
    l.forced_height = c.height

    return wibox.widget {
        {
            l,
            {
                text   = label or "",
                halign = "center",
                valign = "center",
                widget = wibox.widget.textbox
            },
            layout = wibox.layout.stack
        },
        border_width    = bw,
        border_color    = bc,
        shape_clip      = true,
        border_strategy = "inner",
        opacity         = c.opacity,
        fg              = beautiful.fg_normal or "#000000",
        bg              = col,
        shape           = function(cr2, w, h)
            if c.shape then
                c.shape(cr2, w, h)
            else
                return shape.rounded_rect(cr2, w, h, args.radius or 5)
            end
        end,

        forced_width  = geo.width  + 2*bw,
        forced_height = geo.height + 2*bw,
        widget        = wibox.container.background,
    }
end

-- Add all wiboxes

-- Fix the wibox geometries that have a dependency on their content
for _, d in ipairs(drawin.get()) do
    local w = d.get_wibox and d:get_wibox() or nil
    if w then
        -- Force a full layout first as widgets with as the awful.popup have
        -- interdependencies between the content and the container
        if w._apply_size_now then
            w:_apply_size_now()
        end
    end
end

-- Emulate the event loop for another 5 iterations
for _ = 1, 5 do
    require("gears.timer").run_delayed_calls_now()
end

for _, d in ipairs(drawin.get()) do
    local w = d.get_wibox and d:get_wibox() or nil
    if w and w.visible then
        local geo = w:geometry()
        total_area:add_at(w:to_widget(), {x = geo.x, y = geo.y})
    end
end

-- Loop each clients geometry history and paint it
for _, c in ipairs(client.get()) do

    local is_displayed = false

    for _, t in pairs(c:tags()) do
        is_displayed = is_displayed or t.selected
    end

    if (not c.minimized) and is_displayed then
        local pgeo = nil
        for _, geo in ipairs(c._old_geo) do
            if not geo._hide then
                total_area:add_at(
                    client_widget(c, c.color or geo._color or beautiful.bg_normal, geo._label),
                    {x=geo.x, y=geo.y}
                )
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
end

-- Draw the wiboxes/clients on top of the screen
wibox.widget.draw_to_cairo_context(
    total_area, cr, screen._get_extents()
)

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
