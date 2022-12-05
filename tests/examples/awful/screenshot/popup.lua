--DOC_NO_USAGE --DOC_GEN_IMAGE --DOC_HIDE_START
local awful = require("awful")
local wibox = require("wibox")
local gears = require("gears")
local beautiful = require("beautiful")
local s = screen[1]
screen[1]._resize {width = 640, height = 320}

local wb = awful.wibar { position = "top" }

-- Create the same number of tags as the default config
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.layouts[1])

-- Only bother with widgets that are visible by default
local mykeyboardlayout = awful.widget.keyboardlayout()
local mytextclock = wibox.widget.textclock()
local mytaglist = awful.widget.taglist(screen[1], awful.widget.taglist.filter.all, {})
local mytasklist = awful.widget.tasklist(screen[1], awful.widget.tasklist.filter.currenttags, {})

client.connect_signal("request::titlebars", function(c)
    local top_titlebar = awful.titlebar(c, {
        height    = 20,
        bg_normal = beautiful.bg_normal,
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
end)

wb:setup {
    layout = wibox.layout.align.horizontal,
    {
        mytaglist,
        layout = wibox.layout.fixed.horizontal,
    },
    mytasklist,
    {
        layout = wibox.layout.fixed.horizontal,
        mykeyboardlayout,
        mytextclock,
    },
}

require("gears.timer").run_delayed_calls_now()
local counter = 0

local function gen_client(label)
    local c = client.gen_fake {hide_first=true}

    c:geometry {
        x      = 45 + counter*1.75,
        y      = 30 + counter,
        height = 60,
        width  = 230,
    }
    c._old_geo = {c:geometry()}
    c:set_label(label)
    c:emit_signal("request::titlebars")
    c.border_color = beautiful.bg_highlight
    counter = counter + 40

    return c
end

local tasklist_buttons = {}
gen_client("C1")
gen_client("C2")

--DOC_HIDE_END

    awful.popup {
        widget = awful.widget.tasklist {
            screen   = screen[1],
            filter   = awful.widget.tasklist.filter.allscreen,
            buttons  = tasklist_buttons,
            style    = {
                shape = gears.shape.rounded_rect,
                align = "center"
            },
            layout   = {
                spacing   = 5,
                row_count = 1,
                layout    = wibox.layout.grid.horizontal
            },
            widget_template = {
                {
                    {
                        id            = "screenshot",
                        margins       = 4,
                        forced_height = 128,
                        forced_width  = 240,
                        widget        = wibox.container.margin,
                    },
                    {
                        id     = "text_role",
                        forced_height = 20,
                        forced_width  = 240,
                        widget = wibox.widget.textbox,
                    },
                    widget = wibox.layout.fixed.vertical
                },
                id              = "background_role",
                widget          = wibox.container.background,
                create_callback = function(self, c) --luacheck: no unused
                    assert(c) --DOC_HIDE
                    local ss = awful.screenshot {
                        client = c,
                    }
                    ss:refresh()
                    local ib = ss.content_widget
                    ib.valign = "center"
                    ib.halign = "center"
                    self:get_children_by_id("screenshot")[1].widget = ib
                    assert(ss.surface) --DOC_HIDE
                end,
            },
        },
        border_color = "#777777",
        border_width = 2,
        ontop        = true,
        placement    = awful.placement.centered,
        shape        = gears.shape.rounded_rect
    }

--DOC_HIDE_START
awful.wallpaper {
    screen = s,
    widget = {
        image                 = beautiful.wallpaper,
        resize                = true,
        widget                = wibox.widget.imagebox,
        horizontal_fit_policy = "fit",
        vertical_fit_policy   = "fit",
    }
}

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
--DOC_HIDE_END
