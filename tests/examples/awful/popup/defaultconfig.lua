--DOC_GEN_IMAGE
--DOC_HIDE_ALL
--DOC_NO_USAGE
--DOC_NO_DASH
require("_date")
local awful     = require("awful")
local gears     = require("gears")
local naughty   = require("naughty")
local wibox     = require("wibox")
local beautiful = require("beautiful") --DOC_HIDE
local look      = require("_default_look")
local color     = require("gears.color")

screen[1]._resize {width = 640, height = 340}

-- This example is used to show the various type of wibox awesome provides
-- and mimic the default config look

look.mypromptbox.text = "Run:"

local c = client.gen_fake {hide_first=true}

c:geometry {
    x      = 205,
    y      = 260,
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
}

awful.placement.maximize(overlay_w)

local canvas = wibox.layout.manual()
canvas.forced_height = 480
canvas.forced_width  = 640
overlay_w:set_widget(canvas)

local function create_info(text, x, y, width, height)
    canvas:add_at(wibox.widget {
        {
            {
                text = text,
                align = "center",
                ellipsize = "none",
                wrap = "word",
                widget = wibox.widget.textbox
            },
            margins = 10,
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

local function create_line(x1, y1, x2, y2)
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

naughty.connect_signal("request::display", function(n)
    naughty.layout.box {notification = n}
end)

naughty.notification {
    title    = "A notification",
    message  = "With a message! ....",
    position = "top_right",
}

create_info("awful.widget.launcher", 0, 70, 135, 30)
create_info("awful.widget.prompt", 145, 80, 127, 30)
create_info("awful.widget.taglist", 20, 30, 120, 30)
create_info("awful.widget.tasklist", 240, 50, 130, 30)
create_info("wibox.widget.systray", 380, 50, 130, 30)
create_info("awful.widget.keyboardlayout", 315, 15, 170, 30)
create_info("wibox.widget.textclock", 480, 130, 140, 30)
create_info("awful.widget.layoutbox", 490, 170, 150, 30)
create_info("naughty.layout.box", 450, 90, 130, 30)

create_info("awful.titlebar.widget.iconwidget", 10, 260, 190, 30)
create_info("awful.titlebar.widget.titlewidget", 90, 225, 190, 30)
create_info("awful.titlebar.widget.floatingbutton", 150, 190, 205, 30)
create_info("awful.titlebar.widget.maximizedbutton", 150, 155, 220, 30)
create_info("awful.titlebar.widget.stickybutton", 200, 125, 195, 30)
create_info("awful.titlebar.widget.ontopbutton", 390, 210, 200, 30)
create_info("awful.titlebar.widget.closebutton", 445, 260, 190, 30)

create_line(5, 10, 5, 70) --launcher
create_line(75, 10, 75, 30) --taglist
create_line(150, 10, 150, 80) -- prompt
create_line(550, 65, 550, 90) -- notification
create_line(305, 10, 305, 50) -- tasklist
create_line(480, 10, 480, 15) -- keyboard
create_line(600, 5, 600, 130)  --textclock
create_line(630, 5, 630, 170) -- layoutbox
create_line(500, 5, 500, 50) -- systray

create_line(385, 150, 385, 259) -- sticky
create_line(365, 180, 365, 259) -- maximize
create_line(345, 215, 345, 259) -- floating
create_line(405, 235, 405, 259) -- ontop
create_line(195, 270, 203, 270) -- icon
create_line(437, 270, 445, 270) -- close
create_line(245, 250, 245, 259) -- title

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
