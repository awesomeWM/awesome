--DOC_GEN_IMAGE --DOC_HIDE
local awful = { --DOC_HIDE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {clienticon =require("awful.widget.clienticon"), --DOC_HIDE
              tasklist = require("awful.widget.tasklist")} --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

   local previous = nil

   for i=1, 5 do
       local p2 = awful.popup {
           widget = wibox.widget {
               text   = "Hello world!  "..i.."  aaaa.",
               widget = wibox.widget.textbox
           },
           border_color        = beautiful.border_color,
           preferred_positions = "bottom",
           border_width        = 2,
           preferred_anchors   = "back",
           placement           = (not previous) and awful.placement.top or nil,
           offset = {
                y = 10,
           },
       }
       p2:_apply_size_now() --DOC_HIDE
       p2:move_next_to(previous)
       previous = p2
       previous:_apply_size_now() --DOC_HIDE
   end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
