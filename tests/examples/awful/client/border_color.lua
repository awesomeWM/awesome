--DOC_NO_USAGE --DOC_GEN_IMAGE --DOC_ASTERISK
local awful     = require("awful") --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

screen[1]._resize {width = 480, height = 200} --DOC_HIDE

local wb = awful.wibar { position = "top" }--DOC_HIDE

--DOC_HIDE Create the same number of tags as the default config
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.layouts[1]) --DOC_HIDE

--DOC_HIDE Only bother with widgets that are visible by default
local mykeyboardlayout = awful.widget.keyboardlayout() --DOC_HIDE
local mytextclock = wibox.widget.textclock() --DOC_HIDE
local mytaglist = awful.widget.taglist(screen[1], awful.widget.taglist.filter.all, {}) --DOC_HIDE
local mytasklist = awful.widget.tasklist(screen[1], awful.widget.tasklist.filter.currenttags, {}) --DOC_HIDE

client.connect_signal("request::titlebars", function(c)--DOC_HIDE
    local top_titlebar = awful.titlebar(c, {--DOC_HIDE
        height    = 20,--DOC_HIDE
        bg_normal = beautiful.bg_normal,--DOC_HIDE
    })--DOC_HIDE

    top_titlebar : setup {--DOC_HIDE
        { -- Left--DOC_HIDE
            awful.titlebar.widget.iconwidget(c),--DOC_HIDE
            layout  = wibox.layout.fixed.horizontal--DOC_HIDE
        },--DOC_HIDE
        { -- Middle--DOC_HIDE
            { -- Title--DOC_HIDE
                halign = "center",--DOC_HIDE
                widget = awful.titlebar.widget.titlewidget(c)--DOC_HIDE
            },--DOC_HIDE
            layout  = wibox.layout.flex.horizontal--DOC_HIDE
        },--DOC_HIDE
        { -- Right--DOC_HIDE
            awful.titlebar.widget.floatingbutton (c),--DOC_HIDE
            awful.titlebar.widget.maximizedbutton(c),--DOC_HIDE
            awful.titlebar.widget.stickybutton   (c),--DOC_HIDE
            awful.titlebar.widget.ontopbutton    (c),--DOC_HIDE
            awful.titlebar.widget.closebutton    (c),--DOC_HIDE
            layout = wibox.layout.fixed.horizontal()--DOC_HIDE
        },--DOC_HIDE
        layout = wibox.layout.align.horizontal--DOC_HIDE
    }--DOC_HIDE
end)--DOC_HIDE


wb:setup { --DOC_HIDE
    layout = wibox.layout.align.horizontal, --DOC_HIDE
    { --DOC_HIDE
        mytaglist, --DOC_HIDE
        layout = wibox.layout.fixed.horizontal, --DOC_HIDE
    }, --DOC_HIDE
    mytasklist, --DOC_HIDE
    { --DOC_HIDE
        layout = wibox.layout.fixed.horizontal, --DOC_HIDE
        mykeyboardlayout, --DOC_HIDE
        mytextclock, --DOC_HIDE
    }, --DOC_HIDE
} --DOC_HIDE

require("gears.timer").run_delayed_calls_now()--DOC_HIDE

local function gen_client(label)--DOC_HIDE
    local c = client.gen_fake {hide_first=true} --DOC_HIDE

    c:geometry {--DOC_HIDE
        x      = 105,--DOC_HIDE
        y      = 60,--DOC_HIDE
        height = 60,--DOC_HIDE
        width  = 230,--DOC_HIDE
    }--DOC_HIDE
    c._old_geo = {c:geometry()} --DOC_HIDE
    c:set_label(label) --DOC_HIDE
    c:emit_signal("request::titlebars")--DOC_HIDE
    return c --DOC_HIDE
end --DOC_HIDE

    local c = gen_client("A manually set border_color") --DOC_HIDE
    c.border_color = "#ff00ff"

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

