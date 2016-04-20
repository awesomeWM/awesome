screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE

local f = (awful.placement.right + awful.placement.left)
f(client.focus)

assert(c.x == 0 and c.y==screen[1].geometry.height/2-30/2-c.border_width--DOC_HIDE
    and c.width==40 and c.height==30)--DOC_HIDE
