--DOC_GEN_IMAGE
--DOC_HIDE_ALL
--DOC_NO_USAGE
local awful     = require("awful")
local gears     = require("gears")
local wibox     = require("wibox")
local beautiful = require("beautiful") --DOC_HIDE

screen[1]._resize {width = 640, height = 480}

-- This example is used to show the various type of wibox awesome provides
-- and mimic the default config look

local c = client.gen_fake {hide_first=true}

c:geometry {
    x      = 50,
    y      = 350,
    height = 100,
    width  = 150,
}
c._old_geo = {c:geometry()}
c:set_label("A client")

local wb = awful.wibar {
    position = "top",
}

-- Create the same number of tags as the default config
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.layouts[1])

-- Only bother with widgets that are visible by default
local mykeyboardlayout = awful.widget.keyboardlayout()
local mytextclock = wibox.widget.textclock()
local mylayoutbox = awful.widget.layoutbox(screen[1])
local mytaglist = awful.widget.taglist(screen[1], awful.widget.taglist.filter.all, {})
local mytasklist = awful.widget.tasklist(screen[1], awful.widget.tasklist.filter.currenttags, {})

wb:setup {
    layout = wibox.layout.align.horizontal,
    { -- Left widgets
        layout = wibox.layout.fixed.horizontal,
        awful.titlebar.widget.iconwidget(c), --looks close enough
        mytaglist,
    },
    mytasklist, -- Middle widget
    { -- Right widgets
        layout = wibox.layout.fixed.horizontal,
        mykeyboardlayout,
        mytextclock,
        mylayoutbox,
    },
}

-- The popup
awful.popup {
    widget = wibox.widget {
        awful.widget.layoutlist {
            screen      = 1,
            base_layout = wibox.widget {
                spacing         = 5,
                forced_num_cols = 5,
                layout          = wibox.layout.grid.vertical(),
            },
            widget_template = {
                {
                    {
                        id            = 'icon_role',
                        forced_height = 22,
                        forced_width  = 22,
                        widget        = wibox.widget.imagebox,
                    },
                    margins = 4,
                    widget  = wibox.container.margin,
                },
                id              = 'background_role',
                forced_width    = 24,
                forced_height   = 24,
                shape           = gears.shape.rounded_rect,
                widget          = wibox.container.background,
            },
        },
        margins = 4,
        widget  = wibox.container.margin,
    },
    border_color = beautiful.border_color,
    border_width = beautiful.border_width,
    placement    = awful.placement.centered,
    ontop        = true,
    shape        = gears.shape.rounded_rect
}

-- poor copy of awful.widget.calendar_widget until I fix its API to be less
-- insane.
local p10 = awful.popup {
    widget = {
        wibox.widget.calendar.month(os.date('*t')),
        top = 30,
        margins = 10,
        layout = wibox.container.margin
    },
    preferred_anchors   = "middle",
    border_width        = 2,
    border_color        = beautiful.border_color,
    hide_on_right_click = true,
    placement           = function(d) return awful.placement.top_right(d, {
        honor_workarea = true,
    }) end,
    shape = gears.shape.infobubble,
}

awesome.emit_signal("refresh")
p10:bind_to_widget(mytextclock)

-- The titlebar

local top_titlebar = awful.titlebar(c, {
    height    = 20,
    bg_normal = "#ff0000",
})

top_titlebar : setup {
    { -- Left
        awful.titlebar.widget.iconwidget(c),
        layout  = wibox.layout.fixed.horizontal
    },
    { -- Middle
        { -- Title
            align  = "center",
            widget = awful.titlebar.widget.titlewidget(c)
        },
        layout  = wibox.layout.flex.horizontal
    },
    { -- Right
        awful.titlebar.widget.floatingbutton (c),
        awful.titlebar.widget.maximizedbutton(c),
        awful.titlebar.widget.stickybutton   (c),
        awful.titlebar.widget.ontopbutton    (c),
        awful.titlebar.widget.closebutton    (c),
        layout = wibox.layout.fixed.horizontal()
    },
    layout = wibox.layout.align.horizontal
}

-- Normal wiboxes

wibox {
    width = 50,
    height = 50,
    shape = gears.shape.octogon,
    color = "#0000ff",
    x = 570,
    y = 410,
    border_width = 2,
    border_color = beautiful.border_color,
}

-- The tooltip

mouse.coords{x=50, y= 100}
mouse.push_history()

local tt = awful.tooltip {
    text = "A tooltip!",
    visible = true,
}
tt.bg = beautiful.bg_normal

-- Extra information overlay

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
                widget = wibox.widget.textbox
            },
            margins = 10,
            widget = wibox.container.margin
        },
        forced_width  = width,
        forced_height = height,
        shape = gears.shape.rectangle,
        shape_border_width = 1,
        shape_border_color = beautiful.border_color,
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
            cr:set_source_rgb(0,0,0)
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

create_info("awful.wibar", 200, 50, 100, 30)
create_info("awful.titlebar", 250, 350, 100, 30)
create_info("awful.tooltip", 30, 130, 100, 30)
create_info("awful.popup", 450, 240, 100, 30)
create_info("Standard `wibox1`", 420, 420, 130, 30)

create_line(250, 10, 250, 55)
create_line(75, 100, 75, 135)
create_line(545, 432, 575, 432)
create_line(500, 165, 500, 245)
create_line(390, 250, 450, 250)
create_line(190, 365, 255, 365)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
