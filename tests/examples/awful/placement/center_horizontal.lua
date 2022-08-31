--DOC_GEN_IMAGE --DOC_HIDE
-- Align a client to the horizontal center left of the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client`, `mouse` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments") --DOC_HEADER
-- @treturn table The new geometry --DOC_HEADER
-- @staticfct awful.placement.center_horizontal --DOC_HEADER

require("awful.tag").add("1", {screen=screen[1], selected=true}) --DOC_HIDE
screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local awful = {placement = require("awful.placement")} --DOC_HIDE
local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE

awful.placement.center_horizontal(client.focus)

assert(c.x == screen[1].geometry.width/2-40/2-c.border_width)--DOC_HIDE
assert(c.y==35)--DOC_HIDE
assert(c.width==40 and c.height==30)--DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
