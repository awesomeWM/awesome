--DOC_GEN_IMAGE --DOC_HIDE
require("awful.tag").add("1", {screen=screen[1], selected=true}) --DOC_HIDE
screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
client.gen_fake {x = 10, y = 10, width=40, height=30} --DOC_HIDE

mouse.coords {x= 50, y=50} --DOC_HIDE
mouse.push_history() --DOC_HIDE

awful.placement.under_mouse(client.focus)

assert(client.focus.x + client.focus.width /2 - mouse.coords().x <= 1) --DOC_HIDE
assert(client.focus.y + client.focus.height/2 - mouse.coords().y <= 1) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
