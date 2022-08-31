--DOC_GEN_IMAGE
--DOC_HIDE_ALL
--DOC_NO_USAGE
--DOC_NO_DASH
require("_date")
local gears     = require("gears")
local wibox     = require("wibox")
local beautiful = require("beautiful") --DOC_HIDE
local look      = require("_default_look")
local color     = require("gears.color")

local offset = 0

screen[1]._resize {width = 640, height = 240, y=offset}
screen[1]._no_outline = true
look.mywibox.visible = false

local c = client.gen_fake {hide_first=true}

c:geometry {
    x      = 205,
    y      = 110,
    height = 60,
    width  = 230,
}
c._old_geo = {c:geometry()}
c:set_label("A client (window)")

require("gears.timer").run_delayed_calls_now()

-- The titlebar
c:emit_signal("request::titlebars", "rules", {})--DOC_HIDE

local overlay_w = wibox {
    bg = "#00000000",
    visible = true,
    ontop = true,
    y=0,
    x=0,
    width  = screen[1].geometry.width,
    height = screen[1].geometry.height+offset,
}

local canvas = wibox.layout.manual()
canvas.forced_height = 170
canvas.forced_width  = 640
overlay_w:set_widget(canvas)

local function create_info(text, x, y, width, height)
    y = y + offset
    canvas:add_at(wibox.widget {
        {
            {
                text = text,
                align = "center",
                ellipsize = "none",
                wrap = "word",
                widget = wibox.widget.textbox
            },
            margins = 3,
            widget = wibox.container.margin
        },
        forced_width  = width,
        forced_height = height,
        shape = gears.shape.rectangle,
        border_width = 1,
        border_color = beautiful.border_color,
        bg = "#ffff0055",
        widget = wibox.container.background
    }, {x = x, y = y})
end

local function create_section(x1, y, x2, text)
    canvas:add_at(wibox.widget {
        fit = function()
            return x2-x1+6, 10
        end,
        draw = function(_, _, cr)
            cr:set_source_rgb(0.8, 0.6, 1)
            cr:set_line_width(1)
            cr:move_to(1.5, 0)
            cr:line_to(1.5, 10)
            cr:stroke()
            cr:move_to(x2-x1-1.5, 0)
            cr:line_to(x2-x1-1.5, 10)
            cr:stroke()
            cr:move_to(1.5, 5)
            cr:line_to(x2-x1-1.5, 5)
            cr:stroke()
        end,
        layout = wibox.widget.base.make_widget,
    }, {x=x1, y=y})

    canvas:add_at(wibox.widget {
        text = text,
        align = "center",
        ellipsize = "none",
        wrap = "word",
        forced_width = x2-x1,
        widget = wibox.widget.textbox
    }, {x=x1, y=y-12})

end

local function create_line(x1, y1, x2, y2)
    y1, y2 = y1 + offset, y2 + offset
    return canvas:add_at(wibox.widget {
        fit = function()
            return x2-x1+6, y2-y1+6
        end,
        draw = function(_, _, cr)
            cr:set_source(color(beautiful.fg_normal))
            cr:set_line_width(1)
            cr:arc(1.5, 1.5, 1.5, 0, math.pi*2)
            cr:arc(x2-x1+1.5, y2-y1+1.5, 1.5, 0, math.pi*2)
            cr:fill()
            cr:move_to(1.5,1.5)
            cr:line_to(x2-x1+1.5, y2-y1+1.5)
            cr:stroke()
        end,
        layout = wibox.widget.base.make_widget,
    }, {x=x1, y=y1})
end

create_info("awful.titlebar.widget.iconwidget", 10, 110, 190, 17)
create_info("awful.titlebar.widget.titlewidget", 90, 85, 190, 17)
create_info("awful.titlebar.widget.floatingbutton", 150, 65, 205, 17)
create_info("awful.titlebar.widget.maximizedbutton", 150, 45, 220, 17)
create_info("awful.titlebar.widget.stickybutton", 200, 25, 195, 17)
create_info("awful.titlebar.widget.ontopbutton", 390, 85, 200, 17)
create_info("awful.titlebar.widget.closebutton", 445, 110, 190, 17)

create_line(385, 40, 385, 109) -- sticky
create_line(365, 60, 365, 109) -- maximize
create_line(345, 80, 345, 109) -- floating
create_line(405, 100, 405, 109) -- ontop
create_line(195, 120, 203, 120) -- icon
create_line(437, 120, 445, 120) -- close
create_line(245, 100, 245, 109) -- title

create_section(205, 185, 205+230, 'wibox.layout.align.horizontal')
create_section(205, 200, 205+24, '')
create_section(230, 200, 330, '')
create_section(330, 200, 435, '')
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
