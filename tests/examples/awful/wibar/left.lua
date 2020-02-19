--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful     = require("awful") --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

screen[1]._resize {width = 480, height = 480} --DOC_HIDE

    local wb = awful.wibar { position = "left" }

    --DOC_HIDE Create the same number of tags as the default config
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.layouts[1]) --DOC_HIDE

--DOC_HIDE Only bother with widgets that are visible by default
local mykeyboardlayout = awful.widget.keyboardlayout() --DOC_HIDE
local mytextclock = wibox.widget.textclock() --DOC_HIDE
local mytaglist = awful.widget.taglist(screen[1], awful.widget.taglist.filter.all, {}) --DOC_HIDE
local mytasklist = awful.widget.tasklist(screen[1], awful.widget.tasklist.filter.currenttags, {}) --DOC_HIDE
local month_calendar = awful.widget.calendar_popup.month() --DOC_HIDE
month_calendar:attach( mytextclock, "tr" ) --DOC_HIDE

    wb:setup {
        layout = wibox.layout.align.vertical,
        {
            -- Rotate the widgets with the container
            {
                mytaglist,
                direction = 'west',
                widget = wibox.container.rotate
            },
            layout = wibox.layout.fixed.vertical,
        },
        mytasklist,
        {
            layout = wibox.layout.fixed.vertical,
            {
                -- Rotate the widgets with the container
                {
                    mykeyboardlayout,
                    mytextclock,
                    layout = wibox.layout.fixed.horizontal
                },
                direction = 'west',
                widget = wibox.container.rotate
            }
        },
    }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80

