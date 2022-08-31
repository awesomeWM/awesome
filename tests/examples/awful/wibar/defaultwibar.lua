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

local offset = 60

screen[1]._resize {width = 640, height = 140, y=offset}
screen[1]._no_outline = true

-- This example is used to show the various type of wibox awesome provides
-- and mimic the default config look

look.mypromptbox.text = "Run:"

require("gears.timer").run_delayed_calls_now()
-- The titlebar

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

create_info("awful.widget.launcher", 0, 40, 135, 17)
create_info("awful.widget.prompt", 145, 15, 127, 17)
create_info("awful.widget.taglist", 15, 15, 120, 17)
create_info("awful.widget.tasklist", 240, 40, 130, 17)
create_info("wibox.widget.systray", 380, 40, 130, 17)
create_info("awful.widget.keyboardlayout", 315, 15, 170, 17)
create_info("wibox.widget.textclock", 480, 60, 140, 17)
create_info("awful.widget.layoutbox", 490, 80, 150, 17)

create_line(5, 10, 5, 40) --launcher
create_line(75, 10, 75, 15) --taglist
create_line(150, 10, 150, 15) -- prompt
create_line(305, 10, 305, 40) -- tasklist
create_line(480, 10, 480, 15) -- keyboard
create_line(600, 5, 600, 60)  --textclock
create_line(630, 5, 630, 80) -- layoutbox
create_line(500, 5, 500, 40) -- systray

create_section(0, 10, 640, 'wibox.layout.align.horizontal')
create_section(0, 30, 160, 'align first section (left)')
create_section(0, 50, 160, 'wibox.layout.fixed.horizontal')
create_section(165, 30, 460, 'align second section (middle)')
create_section(465, 30, 640, 'align third section (right)')
create_section(465, 50, 640, 'wibox.layout.fixed.horizontal')
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
