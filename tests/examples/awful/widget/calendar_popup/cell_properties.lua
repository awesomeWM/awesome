--DOC_HIDE_START --DOC_GEN_IMAGE
local awful = require("awful")
local wibox = require("wibox")

screen[1]._resize { width = 480, height = 200 }

local wb = awful.wibar { position = "top", }

-- Create the same number of tags as the default config
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.layouts[1])

-- Only bother with widgets that are visible by default
local mykeyboardlayout = awful.widget.keyboardlayout()
local my_text_clock = wibox.widget.textclock()
local mytaglist = awful.widget.taglist(screen[1], awful.widget.taglist.filter.all, {})
local mytasklist = awful.widget.tasklist(screen[1], awful.widget.tasklist.filter.currenttags, {})
--DOC_HIDE_END

local my_header_properties = {
    markup = function(text) return "<i>" .. text .. "</i>" end,
}
local my_options = { style_header = my_header_properties, }
local month_calendar = awful.widget.calendar_popup.month(my_options)
month_calendar:attach(my_text_clock, "tr")

--DOC_HIDE_START
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
        my_text_clock,
    },
}
month_calendar:toggle()

--vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80 --DOC_HIDE_END