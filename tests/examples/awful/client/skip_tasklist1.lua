--DOC_NO_USAGE --DOC_GEN_IMAGE --DOC_ASTERISK --DOC_HIDE_START
local awful     = require("awful")
local wibox     = require("wibox")
local beautiful = require("beautiful")

screen[1]._resize {width = 480, height = 200}

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
    c.name = label
    counter = counter + 40

    return c
end

    local c1 = gen_client("Client 1 (in tasktar)")
    local c2 = gen_client("Client 2 (NOT in taskbar)")
    local c3 = gen_client("Client 3 (in taskbar)")

--DOC_HIDE_END

  c1.skip_taskbar = false
  c2.skip_taskbar = true
  c3.skip_taskbar = false

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

