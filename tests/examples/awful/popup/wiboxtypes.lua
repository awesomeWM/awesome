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
local assets    = require("beautiful.theme_assets")
local look      = require("_default_look")
local color     = require("gears.color")

screen[1]._resize {width = 640, height = 480}

-- This example is used to show the various type of wibox awesome provides
-- and mimic the default config look

local c = client.gen_fake {hide_first=true}

-- Don't use `awful.wallpaper` to avoid rasterizing the background.

local wallpaper = wibox {
    below = true,
    shape = gears.shape.octogon,
    visible = true,
    bg = "#00000000",
}

awful.placement.maximize(wallpaper)

wallpaper.widget = wibox.widget {
    fit = function(_, _, width, height)
        return width, height
    end,
    draw = function(_, _, cr, width, height)
        cr:translate(width - 80, 60)
        assets.gen_awesome_name(
            cr, height - 40, "#ffffff", beautiful.bg_normal, beautiful.bg_normal
        )
    end,
    widget = wibox.widget.base.make_widget()
}

local p = awful.popup {
    widget = wibox.widget {
        { text   = "Item", widget = wibox.widget.textbox },
        {
            {
                text   = "Selected",
                widget = wibox.widget.textbox
            },
            bg = beautiful.bg_highlight,
            widget = wibox.container.background
        },
        { text   = "Item", widget = wibox.widget.textbox },
        forced_width = 100,
        widget = wibox.layout.fixed.vertical
    },
    border_color = beautiful.border_color,
    border_width = 1,
    x = 50,
    y = 200,
}


p:_apply_size_now()
require("gears.timer").run_delayed_calls_now()
p._drawable._do_redraw()

require("gears.timer").delayed_call(function()
    -- Get the 4th textbox
    local list = p:find_widgets(30, 40)
    mouse.coords {x= 120, y=225}
    mouse.push_history()
    local textboxinstance = list[#list]

    local p2 = awful.popup {
        widget = wibox.widget {
            text = "Submenu",
            forced_height = 20,
            forced_width = 70,
            widget = wibox.widget.textbox
        },
        border_color  = beautiful.border_color,
        preferred_positions = "right",
        border_width  = 1,
    }
    p2:move_next_to(textboxinstance, "right")
end)


c:geometry {
    x      = 50,
    y      = 350,
    height = 100,
    width  = 150,
}
c._old_geo = {c:geometry()}
c:set_label("A client")

-- The popup
awful.popup {
    widget = wibox.widget {
        awful.widget.layoutlist {
            filter      = awful.widget.layoutlist.source.for_screen,
            screen      = 1,
            base_layout = wibox.widget {
                spacing         = 5,
                forced_num_cols = 5,
                layout          = wibox.layout.grid.vertical,
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

require("gears.timer").run_delayed_calls_now()
p10:bind_to_widget(look.mytextclock)

-- The titlebar

c:emit_signal("request::titlebars", "rules", {})--DOC_HIDE

-- Normal wiboxes

wibox {
    width = 50,
    height = 50,
    shape = gears.shape.octogon,
    visible = true,
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
    position = "top_middle",
}

create_info("awful.wibar", 100, 50, 100, 30)
create_info("awful.titlebar", 250, 350, 100, 30)
create_info("awful.tooltip", 30, 130, 100, 30)
create_info("awful.popup", 450, 240, 100, 30)
create_info("naughty.layout.box", 255, 110, 130, 30)
create_info("Standard `wibox`", 420, 420, 130, 30)
create_info("awful.wallpaper", 420, 330, 110, 30)
create_info("awful.menu", 60, 270, 80, 30)

create_line(150, 10, 150, 55)
create_line(75, 100, 75, 135)
create_line(545, 432, 575, 432)
create_line(500, 165, 500, 245)
create_line(380, 250, 450, 250)
create_line(190, 365, 255, 365)
create_line(320, 60, 320, 110)
create_line(525, 345, 570, 345)
create_line(100, 240, 100, 270)

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
