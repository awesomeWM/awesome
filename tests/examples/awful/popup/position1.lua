--DOC_GEN_IMAGE --DOC_HIDE
local awful = { --DOC_HIDE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {clienticon =require("awful.widget.clienticon"), --DOC_HIDE
              tasklist = require("awful.widget.tasklist")} --DOC_HIDE
} --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE

local p = awful.popup { --DOC_HIDE
    widget = wibox.widget { --DOC_HIDE
        text = "Parent wibox", --DOC_HIDE
        forced_width = 100, --DOC_HIDE
        forced_height = 100, --DOC_HIDE
        widget = wibox.widget.textbox --DOC_HIDE
    }, --DOC_HIDE
    border_color = "#777777", --DOC_HIDE
    border_width = 2, --DOC_HIDE
    ontop        = true, --DOC_HIDE
    placement    = awful.placement.centered, --DOC_HIDE
} --DOC_HIDE
p:_apply_size_now() --DOC_HIDE

   for _, v in ipairs {"left", "right", "bottom", "top"} do
       local p2 = awful.popup {
           widget = wibox.widget {
               text   = "On the "..v,
               widget = wibox.widget.textbox
           },
           border_color        = "#777777",
           border_width        = 2,
           preferred_positions = v,
           ontop               = true,
       }
       p2:move_next_to(p)
   end

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
