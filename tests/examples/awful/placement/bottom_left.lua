--DOC_GEN_IMAGE --DOC_HIDE
-- Align a client to the bottom left of the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments") --DOC_HEADER
-- @treturn table The new geometry --DOC_HEADER
-- @staticfct awful.placement.bottom_left --DOC_HEADER

screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE

awful.placement.bottom_left(client.focus)

assert( --DOC_HIDE
       c.x == 0 --DOC_HIDE
       and c.y+2*c.border_width == screen[1].geometry.height-30 --DOC_HIDE
       and c.width == 40--DOC_HIDE
       and c.height == 30--DOC_HIDE
       ) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
