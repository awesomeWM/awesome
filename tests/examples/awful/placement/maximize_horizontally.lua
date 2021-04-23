--DOC_GEN_IMAGE --DOC_HIDE
-- Horizontally maximize the drawable in the parent area. --DOC_HEADER
-- @tparam drawable d A drawable (like `client` or `wibox`) --DOC_HEADER
-- @tparam[opt={}] table args Other arguments") --DOC_HEADER
-- @staticfct awful.placement.maximize_horizontally --DOC_HEADER

screen[1]._resize {width = 128, height = 96} --DOC_HIDE
local placement = require("awful.placement") --DOC_HIDE

local c = client.gen_fake {x = 45, y = 35, width=40, height=30} --DOC_HIDE
c:_hide_all() --DOC_HIDE
placement.maximize_horizontally(c)

assert(c.width + 2*c.border_width == screen[1].geometry.width) --DOC_HIDE
assert(c.y == 35) --DOC_HIDE
assert(c.height == 30) --DOC_HIDE

--DOC_HIDE vim: filetype=lua:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
