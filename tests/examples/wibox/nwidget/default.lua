--DOC_GEN_IMAGE --DOC_HIDE_ALL
local parent    = ...
local naughty = require("naughty")
local wibox = require("wibox")
local beautiful = require("beautiful")
local def = require("naughty.widget._default")
local acommon = require("awful.widget.common")
local aplace = require("awful.placement")
local gears = require("gears")
local color = require("gears.color")

beautiful.notification_bg = beautiful.bg_normal

local notif = naughty.notification {
    title   = "A notification",
    message = "This notification has actions!",
    icon    = beautiful.awesome_icon,
    actions = {
        naughty.action {
            name = "Accept",
            icon = beautiful.awesome_icon,
        },
        naughty.action {
            name = "Refuse",
            icon = beautiful.awesome_icon,
        },
        naughty.action {
            name = "Ignore",
            icon = beautiful.awesome_icon,
        },
    }
}

local default = wibox.widget(def)

acommon._set_common_property(default, "notification", notif)

local w, h = default:fit({dpi=96}, 9999, 9999)
default.forced_width = w + 25
default.forced_height = h

local canvas = wibox.layout.manual()
canvas.forced_width  = w + 150
canvas.forced_height = h + 100

canvas:add_at(default, aplace.centered)

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
            top = 2,
            bottom = 2,
            left = 10,
            right = 10,
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

create_info("naughty.widget.background", 10, canvas.forced_height - 30, nil, nil)
create_line(80, canvas.forced_height-55, 80, canvas.forced_height - 30)

create_info("naughty.list.actions", 170, canvas.forced_height - 30, nil, nil)
create_line(200, canvas.forced_height-105, 200, canvas.forced_height - 30)

create_info("naughty.widget.icon", 20, 25, nil, nil)
create_line(80, 40, 80, 60)

create_info("naughty.widget.title", 90, 4, nil, nil)
create_line(140, 20, 140, 60)

create_info("naughty.widget.message", 150, 25, nil, nil)
create_line(210, 40, 210, 75)

parent:add(canvas)
