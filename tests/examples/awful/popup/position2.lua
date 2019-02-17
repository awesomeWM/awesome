--DOC_GEN_IMAGE --DOC_HIDE
local awful = { --DOC_HIDE --DOC_NO_USAGE
    popup = require("awful.popup"), --DOC_HIDE
    placement = require("awful.placement"), --DOC_HIDE
    widget = {clienticon =require("awful.widget.clienticon"), --DOC_HIDE
              tasklist = require("awful.widget.tasklist")} --DOC_HIDE
} --DOC_HIDE
local gears = { shape = require("gears.shape"), timer=require("gears.timer") } --DOC_HIDE
local wibox = require("wibox") --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE

local p = awful.popup { --DOC_HIDE
    widget = wibox.widget { --DOC_HIDE
        { text   = "Item", widget = wibox.widget.textbox }, --DOC_HIDE
        { text   = "Item", widget = wibox.widget.textbox }, --DOC_HIDE
        { text   = "Item", widget = wibox.widget.textbox }, --DOC_HIDE
        { --DOC_HIDE
            { --DOC_HIDE
                text   = "Selected", --DOC_HIDE
                widget = wibox.widget.textbox --DOC_HIDE
            }, --DOC_HIDE
            bg = beautiful.bg_highlight, --DOC_HIDE
            widget = wibox.container.background --DOC_HIDE
        }, --DOC_HIDE
        { text   = "Item", widget = wibox.widget.textbox }, --DOC_HIDE
        { text   = "Item", widget = wibox.widget.textbox }, --DOC_HIDE
        forced_width = 100, --DOC_HIDE
        widget = wibox.layout.fixed.vertical --DOC_HIDE
    }, --DOC_HIDE
    border_color = "#ff0000", --DOC_HIDE
    border_width = 2, --DOC_HIDE
    placement    = awful.placement.centered, --DOC_HIDE
} --DOC_HIDE
p:_apply_size_now() --DOC_HIDE
require("gears.timer").run_delayed_calls_now() --DOC_HIDE
p._drawable._do_redraw() --DOC_HIDE

--DOC_HIDE Necessary as the widgets are drawn later
gears.timer.delayed_call(function() --DOC_HIDE
-- Get the 4th textbox --DOC_HIDE
local list = p:find_widgets(30, 40) --DOC_HIDE
mouse.coords {x= 120, y=125} --DOC_HIDE
mouse.push_history() --DOC_HIDE
local textboxinstance = list[#list] --DOC_HIDE

   for _, v in ipairs {"left", "right"} do
       local p2 = awful.popup {
           widget = wibox.widget {
               text = "On the "..v,
               forced_height = 100,
               widget = wibox.widget.textbox
           },
           border_color  = "#0000ff",
           preferred_positions = v,
           border_width  = 2,
       }
       p2:move_next_to(textboxinstance, v)
   end
   end) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
