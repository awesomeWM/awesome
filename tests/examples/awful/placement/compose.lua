--DOC_GEN_IMAGE --DOC_HIDE
screen[1]._resize {x = 175, width = 128, height = 96} --DOC_NO_USAGE --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
local c = client.gen_fake {x = 220, y = 35, width=40, height=30} --DOC_HIDE

    -- "right" will be replaced by "left"
    local f = (awful.placement.right + awful.placement.left)
    f(client.focus)

local sg = screen[1].geometry--DOC_HIDE
assert(c.x == sg.x and c.y==sg.height/2-30/2-c.border_width--DOC_HIDE
       and c.width==40 and c.height==30)--DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
