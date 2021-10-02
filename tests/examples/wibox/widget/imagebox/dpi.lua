--DOC_GEN_IMAGE --DOC_HIDE
local parent = ... --DOC_HIDE
local wibox  = require("wibox") --DOC_HIDE

local l = wibox.layout { --DOC_HIDE
    forced_width  = 360, --DOC_HIDE
    spacing       = 5, --DOC_HIDE
    layout        = wibox.layout.flex.vertical --DOC_HIDE
} --DOC_HIDE



   local image = '<?xml version="1.0" encoding="UTF-8" standalone="no"?>'..
       '<svg width="2in" height="1in">'..
           '<rect height="0.1in" width="0.1in" style="fill:red;" />'..
           '<text x="10" y="32" width="150" style="font-size: 0.1in;">Hello world!</text>'..
       '</svg>'

--DOC_NEWLINE

    for _, dpi in ipairs {100, 200, 300} do
       local row = wibox.layout { --DOC_HIDE
           spacing = 5, --DOC_HIDE
           layout = wibox.layout.fixed.horizontal --DOC_HIDE
       } --DOC_HIDE

       row:add(wibox.widget { --DOC_HIDE
           markup = "<b>dpi = "..dpi.."</b>", --DOC_HIDE
           forced_width = 80, --DOC_HIDE
           widget = wibox.widget.textbox --DOC_HIDE
       }) --DOC_HIDE

        local w = wibox.widget {
           image         = image,
           dpi           = dpi,
           resize        = false,
           forced_height = 70,
           forced_width  = 150,
           widget        = wibox.widget.imagebox
       }

       row:add(w) --DOC_HIDE
       l:add(row) --DOC_HIDE
    end

parent:add(l) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
