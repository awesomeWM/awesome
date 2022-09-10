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
                halign = "center",
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
    counter = counter + 40
    c.class = label

    return c
end

    local c1 = gen_client("Inactive")
    local c2 = gen_client("Urgent")
    local c3 = gen_client("Focus")
    local c4 = gen_client("Tag")

    c4:tags{screen[1].tags[2]}
--DOC_HIDE_END

  -- Affects mostly the taglist and tasklist..
  beautiful.fg_urgent = "#ffffff"
  beautiful.bg_urgent = "#ff0000"

  --DOC_NEWLINE

  -- Set the client border to be orange and large.
  beautiful.border_color_urgent = "#ffaa00"
  beautiful.border_width_urgent = 6

  --DOC_NEWLINE

  -- Set the titlebar green.
  beautiful.titlebar_bg_urgent = "#00ff00"
  beautiful.titlebar_fg_urgent = "#000000"

  --DOC_NEWLINE

  -- This client is in the current tag.
  c2.urgent = true

  --DOC_NEWLINE

  -- This client is in a deselected tag.
  c4.urgent = true

--DOC_HIDE_START
  c1:emit_signal("request::titlebars")
  c2:emit_signal("request::titlebars")
  c3:emit_signal("request::titlebars")

return {honor_titlebar_colors = true}
--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

