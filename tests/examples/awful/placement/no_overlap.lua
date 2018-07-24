--DOC_GEN_IMAGE --DOC_HIDE
screen[1]._resize {width = 128, height = 96} --DOC_HIDE
screen._add_screen {x = 140, y = 0  , width = 128, height = 96} --DOC_HIDE
screen._add_screen {x = 0  , y = 110, width = 128, height = 96} --DOC_HIDE
screen._add_screen {x = 140, y = 110, width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
local beautiful = require("beautiful") --DOC_HIDE


client.gen_fake {x = 10, y = 10, width=40, height=30} --DOC_HIDE
client.gen_fake {x = 80, y = 55, width=40, height=30} --DOC_HIDE
client.gen_fake {x = 0, y = 0, width=40, height=50, color=beautiful.bg_highlight} --DOC_HIDE
client.focus:_hide() --DOC_HIDE

awful.placement.no_overlap(client.focus)

local x,y = screen[4].geometry.x, screen[4].geometry.y
client.gen_fake {x = x+10, y = y+10, width=40, height=30} --DOC_HIDE
client.gen_fake {x = x+80, y = y+10, width=40, height=30} --DOC_HIDE
client.gen_fake {x = x+10, y = y+55, width=40, height=30} --DOC_HIDE
client.gen_fake {x = x+80, y = y+55, width=40, height=30} --DOC_HIDE
client.gen_fake {x = x+0, y = y+0, width=40, height=50, color=beautiful.bg_highlight} --DOC_HIDE
client.focus:_hide() --DOC_HIDE
awful.placement.no_overlap(client.focus) --FIXME --DOC_HIDE

--TODO maximized + no_overlap --DOC_HIDE
--TODO add 9 clients with no_overlap on all of them --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
