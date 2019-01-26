--DOC_NO_USAGE --DOC_GEN_IMAGE
local awful     = require("awful") --DOC_HIDE
local wibox     = require("wibox") --DOC_HIDE

screen[1]._resize {width = 480, height = 100} --DOC_HIDE

local wb = awful.wibar { position = "top", } --DOC_HIDE

--DOC_HIDE Create the same number of tags as the default config
awful.tag({ "1", "2", "3", "4", "5", "6", "7", "8", "9" }, screen[1], awful.layout.layouts[1]) --DOC_HIDE

--DOC_HIDE Only bother with widgets that are visible by default
local mykeyboardlayout = awful.widget.keyboardlayout() --DOC_HIDE
local mytextclock = wibox.widget.textclock() --DOC_HIDE
local mylayoutbox = awful.widget.layoutbox(screen[1]) --DOC_HIDE
local mytaglist = awful.widget.taglist(screen[1], awful.widget.taglist.filter.all, {}) --DOC_HIDE
local mytasklist = awful.widget.tasklist(screen[1], awful.widget.tasklist.filter.currenttags, {}) --DOC_HIDE

   wb:setup {
       layout = wibox.layout.align.horizontal,
       { -- Left widgets
           mytaglist,
           awful.widget.layoutlist {
               screen = 1,
               style = {
                   disable_name = true,
                   spacing      = 3,
               },
               source = function() return {
                   awful.layout.suit.floating,
                   awful.layout.suit.tile,
                   awful.layout.suit.tile.left,
                   awful.layout.suit.tile.bottom,
                   awful.layout.suit.tile.top,
               } end
           },
           layout = wibox.layout.fixed.horizontal,
       },
       mytasklist, -- Middle widget
       { -- Right widgets
           layout = wibox.layout.fixed.horizontal,
           mykeyboardlayout,
           mytextclock,
           mylayoutbox,
       },
   }

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
