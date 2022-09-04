local awful     = require("awful")
local wibox     = require("wibox")
local beautiful = require("beautiful")
require("_date")

local ret = nil

for i = 1, screen.count() do
    local s = screen[i]

    -- Create the same number of tags as the default config
    awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, s, awful.layout.layouts[1])
    local mykeyboardlayout = awful.widget.keyboardlayout()
    local mytextclock = wibox.widget.textclock()
    local mylayoutbox = awful.widget.layoutbox(s)
    local mytaglist = awful.widget.taglist(s, awful.widget.taglist.filter.all, {})
    local mytasklist = awful.widget.tasklist(s, awful.widget.tasklist.filter.currenttags, {})
    local mypromptbox = wibox.widget.textbox("")

    local wb = awful.wibar {
        position = "top",
        screen   = s
    }

    wb:setup {
        layout = wibox.layout.align.horizontal,
        {
            layout = wibox.layout.fixed.horizontal,
            {
                image  = beautiful.awesome_icon,
                widget = wibox.widget.imagebox,
            },
            mytaglist,
            mypromptbox,
        },
        mytasklist,
        {
            layout = wibox.layout.fixed.horizontal,
            mykeyboardlayout,
            {
                image  = beautiful.awesome_icon,
                widget = wibox.widget.imagebox,
            },
            {
                image  = beautiful.awesome_icon,
                widget = wibox.widget.imagebox,
            },
            {
                image  = beautiful.awesome_icon,
                widget = wibox.widget.imagebox,
            },
            mytextclock,
            mylayoutbox,
        },
    }

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

    ret = ret or {
        mykeyboardlayout = mykeyboardlayout,
        mytextclock      = mytextclock     ,
        mylayoutbox      = mylayoutbox     ,
        mytaglist        = mytaglist       ,
        mytasklist       = mytasklist      ,
        mywibox          = wb              ,
        mypromptbox      = mypromptbox     ,
    }
end

require("gears.timer").run_delayed_calls_now()

return ret
