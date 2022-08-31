--DOC_HIDE_ALL
--DOC_GEN_IMAGE
local placement = require("awful.placement")
require("awful.tag").add("1", {screen=screen[1], selected=true}) --DOC_HIDE
screen[1]._resize {x= 50}
for _, pos in ipairs{
    "top_left", "top_right", "bottom_left", "bottom_right",
    "left", "right", "top", "bottom", "centered",
} do
local c1 = client.gen_fake {x = 80, y = 55, width=75, height=50}
c1:_hide_all()
placement.align(client.focus, {position = pos, honor_workarea=true})
c1:set_label(pos)
end

screen._add_screen {x = 70, y = 260  , width = 128, height = 96}
screen._add_screen {x = 210, y = 260  , width = 128, height = 96}
require("awful.tag").add("1", {screen=screen[2], selected=true}) --DOC_HIDE
require("awful.tag").add("1", {screen=screen[3], selected=true}) --DOC_HIDE

local c

c = client.gen_fake {x = screen[2].geometry.x+10, y = screen[2].geometry.y+10, width=40, height=30, screen=screen[2]}
placement.align(c,{position = "center_vertical", honor_workarea = true})

c = client.gen_fake {x = screen[3].geometry.x+10, y = screen[3].geometry.y+10, width=40, height=30, screen=screen[3]}
placement.align(c, {position = "center_horizontal", honor_workarea=true})

return {hide_lines=true}

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
