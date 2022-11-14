--DOC_NO_USAGE --DOC_GEN_IMAGE --DOC_HIDE_START
local awful = require("awful")
local wibox = require("wibox")
local beautiful = require("beautiful")
local naughty = require("naughty")
local s = screen[1]
screen[1]._resize {width = 640, height = 320}

local wb = awful.wibar { position = "top" }
local modkey = "Mod4"

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

gen_client("C1")
gen_client("C2")

--DOC_HIDE_END


    local function saved_screenshot(args)
        local ss = awful.screenshot(args)

        --DOC_NEWLINE
        local function notify(self)
            naughty.notification {
                title     = self.file_name,
                message   = "Screenshot saved",
                icon      = self.surface,
                icon_size = 128,
            }
        end
        --DOC_NEWLINE

        if args.auto_save_delay > 0 then
            ss:connect_signal("file::saved", notify)
        else
            notify(ss)
        end
        --DOC_NEWLINE

        return ss
    end
    --DOC_NEWLINE
    local function delayed_screenshot(args)
        local ss = saved_screenshot(args)
        local notif = naughty.notification {
            title   = "Screenshot in:",
            message = tostring(args.auto_save_delay) .. " seconds"
        }
        --DOC_NEWLINE

        ss:connect_signal("timer::tick", function(_, remain)
            notif.message = tostring(remain) .. " seconds"
        end)
        --DOC_NEWLINE

        ss:connect_signal("timer::timeout", function()
            if notif then notif:destroy() end
        end)
        --DOC_NEWLINE

        return ss
    end
    --DOC_NEWLINE
    client.connect_signal("request::default_keybindings", function()
        awful.keyboard.append_client_keybindings({
            awful.key({modkey}, "Print",
                function (c) saved_screenshot { auto_save_delay = 0, client = c } end ,
                {description = "take client screenshot", group = "client"}),
            awful.key({modkey, "Shift"}, "Print",
                function (c) saved_screenshot { auto_save_delay = 0, interactive = true, client = c } end ,
                {description = "take interactive client screenshot", group = "client"}),
            awful.key({modkey, "Control"}, "Print",
                function (c) delayed_screenshot { auto_save_delay = 5, client = c } end ,
                {description = "take screenshot in 5 seconds", group = "client"}),
            awful.key({modkey, "Shift", "Control"}, "Print",
                function (c) delayed_screenshot { auto_save_delay = 5, interactive = true, client = c } end ,
                {description = "take interactive screenshot in 5 seconds", group = "client"}),
        })
    end)
    --DOC_NEWLINE

    awful.keyboard.append_global_keybindings({
        awful.key({}, "Print",
            function () saved_screenshot { auto_save_delay = 0 } end ,
            {description = "take screenshot", group = "client"}),
        awful.key({"Shift"}, "Print",
            function () saved_screenshot { auto_save_delay = 0, interactive = true } end ,
            {description = "take interactive screenshot", group = "client"}),
        awful.key({"Control"}, "Print",
            function () delayed_screenshot { auto_save_delay = 5 } end ,
            {description = "take screenshot in 5 seconds", group = "client"}),
        awful.key({"Shift", "Control"}, "Print",
            function () delayed_screenshot { auto_save_delay = 5, interactive = true } end ,
            {description = "take interactive screenshot in 5 seconds", group = "client"}),
    })

--DOC_HIDE_START

client.emit_signal("request::default_keybindings")

-- A notification popup using the default widget_template.
naughty.connect_signal("request::display", function(n)
    naughty.layout.box {notification = n}
end)

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

saved_screenshot { auto_save_delay = 0 }
delayed_screenshot { auto_save_delay = 5 }

require("gears.timer").run_delayed_calls_now() --DOC_HIDE
--DOC_HIDE_END



