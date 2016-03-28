screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
client.gen_fake {x = 10, y = 10, width=40, height=30} --DOC_HIDE

mouse.coords {x= 50, y=50} --DOC_HIDE
mouse.push_history() --DOC_HIDE

awful.placement.under_mouse(client.focus)
